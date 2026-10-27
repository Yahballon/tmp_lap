#!/usr/bin/env python3
"""Test SDK mirror rewrite without network access or downloading binary assets."""
from pathlib import Path
from tempfile import TemporaryDirectory
import json
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from fix_opengl_sdk_mirror import (EXPECTED_SHA256, ORIGINAL, MIRROR, switch_source)


def run():
    with TemporaryDirectory() as root:
        folder = Path(root)
        (folder / "tools").mkdir()
        target = folder / "tools" / "deps.json"
        sample = {"items": [
            {"name": "other", "url": "https://example.invalid/other.tar.gz", "sha256": "foo"},
            {"name": "opengl-sdk", "kind": "archive", "url": ORIGINAL,
             "sha256": EXPECTED_SHA256, "creates": ".deps/ps5-opengl-sdk-1.0.2"}
        ]}
        target.write_text(json.dumps(sample), encoding="utf-8")
        assert "matching-SHA-256" in switch_source(folder)
        resulting = json.loads(target.read_text())
        assert resulting["items"][1]["url"] == MIRROR
        assert resulting["items"][1]["sha256"] == EXPECTED_SHA256
        assert resulting["items"][0] == sample["items"][0]
        assert "already pinned" in switch_source(folder)
        resulting["items"][1]["sha256"] = "0" * 64
        target.write_text(json.dumps(resulting), encoding="utf-8")
        try:
            switch_source(folder)
        except ValueError as exc:
            assert "digest changed" in str(exc)
        else:
            raise AssertionError("Changed SHA unexpectedly accepted")
    print("PASS: SDK mirror rewrite preserves upstream SHA pin")


if __name__ == "__main__":
    run()
