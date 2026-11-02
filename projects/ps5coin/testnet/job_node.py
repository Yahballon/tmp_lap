#!/usr/bin/env python3
"""PS5Coin mining-job interoperability prototype. NOT a cryptocurrency node.

Uses REAL RandomX v1.2.1 hashes and a single local tip. No P2P, consensus,
signatures, wallet, balances, payouts, or persistent authoritative ledger.
The testnet work protocol is experimental and MUST NOT be used with funds.
"""
from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from secrets import token_hex
import struct
from threading import RLock
from time import time
from urllib.parse import parse_qs, urlsplit

HEADER_MAGIC = b"P5T1"
GENESIS = hashlib.sha256(b"PS5Coin prototype genesis: DO NOT USE FOR MAINNET").digest()
SEED = b"PS5Coin v0.0.1 - fixed development RandomX seed - not mainnet"
MAX_MINER_LEN = 64
MAX_JOB_AGE = 120
MAX_BODY_BYTES = 4096


class RX:
    """C API from the unmodified upstream RandomX shared library, light mode."""
    def __init__(self, path: str | None = None):
        path = path or os.getenv("RANDOMX_LIB", "librandomx.so")
        self._lib = ctypes.CDLL(path)
        lib = self._lib
        lib.randomx_alloc_cache.argtypes = [ctypes.c_uint32]
        lib.randomx_alloc_cache.restype = ctypes.c_void_p
        lib.randomx_init_cache.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
        lib.randomx_create_vm.argtypes = [ctypes.c_uint32, ctypes.c_void_p, ctypes.c_void_p]
        lib.randomx_create_vm.restype = ctypes.c_void_p
        lib.randomx_calculate_hash.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p]
        lib.randomx_destroy_vm.argtypes = [ctypes.c_void_p]
        lib.randomx_release_cache.argtypes = [ctypes.c_void_p]
        self._lock = RLock()
        self._cache = lib.randomx_alloc_cache(0)
        if not self._cache:
            raise MemoryError("RandomX cache allocation failed (requires about 256 MiB)")
        key = ctypes.create_string_buffer(SEED)
        lib.randomx_init_cache(self._cache, key, len(SEED))
        self._vm = lib.randomx_create_vm(0, self._cache, None)
        if not self._vm:
            lib.randomx_release_cache(self._cache)
            self._cache = None
            raise RuntimeError("RandomX interpreter VM creation failed")

    def digest(self, header: bytes) -> bytes:
        # A VM is not thread-safe; serialize all incoming request verifications.
        with self._lock:
            inp = ctypes.create_string_buffer(header, len(header))
            out = ctypes.create_string_buffer(32)
            self._lib.randomx_calculate_hash(self._vm, inp, len(header), out)
            return out.raw

    def close(self):
        with self._lock:
            if self._vm:
                self._lib.randomx_destroy_vm(self._vm)
                self._vm = None
            if self._cache:
                self._lib.randomx_release_cache(self._cache)
                self._cache = None


def prefix(height: int, prev: bytes, timestamp: int, miner: str) -> bytes:
    """Canonical bytes shared by the node and eventual PS5 miner.

    Nonce is a final unsigned 64-bit LITTLE-ENDIAN integer; not a JSON hash.
    """
    if len(prev) != 32 or not isinstance(miner, str) or not 1 <= len(miner) <= MAX_MINER_LEN:
        raise ValueError("invalid previous hash or miner label")
    if not miner.isascii() or not all(c.isalnum() or c in "-_" for c in miner):
        raise ValueError("miner label must use ASCII letters, digits, - or _")
    if not 0 <= height < 2 ** 64 or not 0 <= timestamp < 2 ** 64:
        raise ValueError("height or timestamp out of bounds")
    return HEADER_MAGIC + struct.pack("<QQ", height, timestamp) + prev + hashlib.sha256(miner.encode("ascii")).digest()


def make_header(pre_nonce: bytes, nonce: int) -> bytes:
    if type(nonce) is not int or not 0 <= nonce < 2 ** 64:
        raise ValueError("nonce must be unsigned 64-bit integer")
    return pre_nonce + struct.pack("<Q", nonce)


def target_for_bits(bits: int) -> int:
    if not 0 <= bits <= 255:
        raise ValueError("invalid difficulty bits")
    return (1 << (256 - bits)) - 1


class Node:
    def __init__(self, rx: RX, difficulty_bits: int = 4):
        if not 0 <= difficulty_bits <= 24:
            raise ValueError("prototype difficulty must be 0..24 bits")
        self.rx = rx
        self.bits = difficulty_bits
        self.target = target_for_bits(difficulty_bits)
        self._lock = RLock()
        self.chain: list[dict] = []
        self.jobs: dict[str, dict] = {}
        self.tip = GENESIS

    def job(self, miner: str) -> dict:
        with self._lock:
            height = len(self.chain) + 1
            stamp = int(time())
            p = prefix(height, self.tip, stamp, miner)
            job_id = token_hex(16)
            self.jobs[job_id] = dict(prefix=p, miner=miner, height=height, prev=self.tip, issued=time(), stamp=stamp)
            # Avoid unbounded storage from hostile job polling.
            if len(self.jobs) > 128:
                self.jobs = {job_id: self.jobs[job_id]}
            return dict(job_id=job_id, height=height, prev_hash=self.tip.hex(),
                        header_prefix_hex=p.hex(), nonce_encoding="uint64_le",
                        randomx_seed_hex=SEED.hex(), target_hex=f"{self.target:064x}",
                        difficulty_bits=self.bits, expires_seconds=MAX_JOB_AGE,
                        protocol="ps5coin-pow-prototype-v0.0.1")

    def submit(self, job_id: str, nonce: int) -> dict:
        with self._lock:
            j = self.jobs.get(job_id)
            if not j:
                return dict(accepted=False, reason="unknown_job")
            if time() - j["issued"] > MAX_JOB_AGE:
                self.jobs.pop(job_id, None)
                return dict(accepted=False, reason="expired_job")
            if j["height"] != len(self.chain) + 1 or j["prev"] != self.tip:
                return dict(accepted=False, reason="stale_job")
            try:
                data = make_header(j["prefix"], nonce)
            except ValueError:
                return dict(accepted=False, reason="invalid_nonce")
            digest = self.rx.digest(data)
            if int.from_bytes(digest, "big") > self.target:
                return dict(accepted=False, reason="insufficient_work", digest=digest.hex())
            block = dict(height=j["height"], prev_hash=j["prev"].hex(),
                         timestamp=j["stamp"], miner=j["miner"], nonce=nonce,
                         digest=digest.hex(), header_hex=data.hex())
            self.chain.append(block)
            self.tip = digest
            self.jobs.clear()
            return dict(accepted=True, block=block)

    def status(self) -> dict:
        with self._lock:
            return dict(network="LOCAL PROTOTYPE ONLY", height=len(self.chain),
                        tip=self.tip.hex(), difficulty_bits=self.bits,
                        p2p_enabled=False, wallet_enabled=False,
                        consensus_finalized=False)


def handler_for(node: Node):
    class Handler(BaseHTTPRequestHandler):
        def answer(self, status, payload):
            data = json.dumps(payload, separators=(",", ":")).encode()
            self.send_response(status)
            self.send_header("Content-Type", "application/json")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)

        def do_GET(self):
            u = urlsplit(self.path)
            if u.path == "/status":
                return self.answer(200, node.status())
            if u.path == "/job":
                miner = parse_qs(u.query).get("miner", ["ps5miner"])[0]
                try:
                    return self.answer(200, node.job(miner))
                except ValueError as e:
                    return self.answer(400, dict(error=str(e)))
            return self.answer(404, dict(error="unknown_endpoint"))

        def do_POST(self):
            if self.path != "/submit":
                return self.answer(404, dict(error="unknown_endpoint"))
            try:
                count = int(self.headers.get("Content-Length", "0"))
                if count < 1 or count > MAX_BODY_BYTES:
                    return self.answer(413, dict(error="invalid_body_size"))
                d = json.loads(self.rfile.read(count))
                if not isinstance(d, dict):
                    raise ValueError("expected object")
                if type(d.get("job_id")) is not str or len(d["job_id"]) != 32:
                    raise ValueError("invalid job_id")
                nonce = d.get("nonce")
                if type(nonce) is not int or not 0 <= nonce < 2 ** 64:
                    raise ValueError("invalid nonce")
                result = node.submit(d["job_id"], nonce)
                return self.answer(200 if result["accepted"] else 422, result)
            except (ValueError, UnicodeDecodeError, json.JSONDecodeError):
                return self.answer(400, dict(error="invalid_json_or_nonce"))

        def log_message(self, format, *args):
            return

    return Handler


def main():
    ap = argparse.ArgumentParser(description="PS5Coin RandomX work protocol prototype (not a chain)")
    ap.add_argument("--host", default="127.0.0.1", help="loopback by default; bind LAN only on trusted network")
    ap.add_argument("--port", default=19444, type=int)
    ap.add_argument("--difficulty-bits", default=4, type=int)
    ap.add_argument("--randomx-lib", default=None)
    args = ap.parse_args()
    rx = RX(args.randomx_lib)
    node = Node(rx, args.difficulty_bits)
    server = ThreadingHTTPServer((args.host, args.port), handler_for(node))
    print(f"PS5Coin DEV ONLY RandomX jobs on http://{args.host}:{args.port}", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
        rx.close()


if __name__ == "__main__":
    main()
