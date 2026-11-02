#!/usr/bin/env python3
"""Integration tests: real libRandomX, canonical block header, job submission.

No mock hashing. To keep CI fast the easy acceptance test uses 0 PoW bits.
The rejection test uses 12 bits and selects an actually insufficient nonce.
"""
import json
from pathlib import Path
import sys
from threading import Thread
import unittest
from urllib.error import HTTPError
from urllib.request import Request, urlopen

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from job_node import RX, Node, SEED, GENESIS, handler_for, make_header, prefix, target_for_bits
from http.server import ThreadingHTTPServer


class PoWTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rx = RX()

    @classmethod
    def tearDownClass(cls):
        cls.rx.close()

    def test_canonical_header_and_nonce(self):
        p = prefix(1, GENESIS, 123456789, "ps5_01")
        self.assertEqual(len(p), 84)
        self.assertEqual(len(make_header(p, 42)), 92)
        self.assertEqual(make_header(p, 42)[-8:], b"\x2a\x00\x00\x00\x00\x00\x00\x00")
        self.assertNotEqual(make_header(p, 42), make_header(p, 43))
        self.assertEqual(SEED, bytes.fromhex(SEED.hex()))
        for invalid in [-1, 2**64, True, 1.5]:
            with self.assertRaises(ValueError):
                make_header(p, invalid)

    def test_real_randomx_job_accepted_and_tip_advances(self):
        node = Node(self.rx, difficulty_bits=0)
        j = node.job("PS5Miner1")
        self.assertEqual(j["height"], 1)
        header = make_header(bytes.fromhex(j["header_prefix_hex"]), 7)
        digest = self.rx.digest(header).hex()
        got = node.submit(j["job_id"], 7)
        self.assertTrue(got["accepted"], got)
        self.assertEqual(got["block"]["digest"], digest)
        self.assertEqual(got["block"]["header_hex"], header.hex())
        self.assertEqual(node.status()["height"], 1)
        self.assertEqual(node.status()["tip"], digest)
        self.assertEqual(node.submit(j["job_id"], 7)["reason"], "unknown_job")
        next_job = node.job("PS5Miner2")
        self.assertEqual(next_job["height"], 2)
        self.assertEqual(next_job["prev_hash"], digest)

    def test_server_rejects_insufficient_work_with_real_digest(self):
        node = Node(self.rx, difficulty_bits=12)
        job = node.job("test_ps5")
        header_prefix = bytes.fromhex(job["header_prefix_hex"])
        nonce = next(n for n in range(20) if int.from_bytes(
            self.rx.digest(make_header(header_prefix, n)), "big") > node.target)
        result = node.submit(job["job_id"], nonce)
        self.assertEqual(result["reason"], "insufficient_work")
        self.assertFalse(result["accepted"])
        self.assertEqual(node.status()["height"], 0)

    def test_http_round_trip(self):
        node = Node(self.rx, 0)
        server = ThreadingHTTPServer(("127.0.0.1", 0), handler_for(node))
        thread = Thread(target=server.serve_forever, daemon=True)
        thread.start()
        base = f"http://127.0.0.1:{server.server_port}"
        try:
            with urlopen(base + "/job?miner=ps5_test", timeout=10) as r:
                job = json.load(r)
            body = json.dumps(dict(job_id=job["job_id"], nonce=123)).encode()
            request = Request(base + "/submit", body,
                              {"Content-Type": "application/json"}, method="POST")
            with urlopen(request, timeout=15) as r:
                result = json.load(r)
            self.assertTrue(result["accepted"])
            with urlopen(base + "/status", timeout=10) as r:
                status = json.load(r)
            self.assertEqual(status["height"], 1)
            self.assertFalse(status["p2p_enabled"])
            self.assertFalse(status["wallet_enabled"])
            try:
                urlopen(base + "/job?miner=bad%20name", timeout=10)
                self.fail("Expected HTTP 400")
            except HTTPError as exc:
                self.assertEqual(exc.code, 400)
        finally:
            server.shutdown()
            thread.join(10)
            server.server_close()


if __name__ == "__main__":
    unittest.main()
