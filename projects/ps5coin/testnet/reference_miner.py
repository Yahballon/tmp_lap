#!/usr/bin/env python3
"""Local reference miner for the PS5Coin RandomX interoperability prototype.

A reference PC miner, NOT the PS5 homebrew and NOT cryptocurrency payouts.
Run against a node on your own PC/LAN. The nonce/header scheme is deliberately
small so that the same exact wire format can be implemented in native PS5 C++.
"""
from __future__ import annotations

import argparse
import json
from time import monotonic
from urllib.error import HTTPError
from urllib.parse import urlencode
from urllib.request import urlopen, Request

from job_node import RX, SEED, make_header


def get_json(url: str) -> dict:
    with urlopen(url, timeout=10) as r:
        return json.load(r)


def submit_json(url: str, payload: dict) -> dict:
    body = json.dumps(payload).encode()
    with urlopen(Request(url, body, {"Content-Type": "application/json"}, method="POST"), timeout=20) as r:
        return json.load(r)


def main():
    ap = argparse.ArgumentParser(description="PS5Coin reference RandomX nonce-search client")
    ap.add_argument("--url", default="http://127.0.0.1:19444")
    ap.add_argument("--miner", default="reference_pc")
    ap.add_argument("--max-seconds", type=int, default=90)
    args = ap.parse_args()
    base = args.url.rstrip("/")
    url = f"{base}/job?{urlencode({'miner': args.miner})}"
    job = get_json(url)
    if bytes.fromhex(job["randomx_seed_hex"]) != SEED:
        raise SystemExit("Node seed differs from supported development seed")
    header_prefix = bytes.fromhex(job["header_prefix_hex"])
    target = int(job["target_hex"], 16)
    rx = RX()
    started = monotonic()
    try:
        nonce = 0
        while monotonic() - started < args.max_seconds:
            digest = rx.digest(make_header(header_prefix, nonce))
            if int.from_bytes(digest, "big") <= target:
                print(f"Found nonce={nonce}, RandomX digest={digest.hex()}, hashes={nonce+1}", flush=True)
                result = submit_json(base + "/submit", dict(job_id=job["job_id"], nonce=nonce))
                print(json.dumps(result, indent=2))
                return
            nonce += 1
        raise SystemExit("No winning nonce in time window; retry or lower development difficulty")
    finally:
        rx.close()


if __name__ == "__main__":
    main()
