#!/usr/bin/env python3
"""Regression: dependency substitutions may never change the expected bytes."""
from pathlib import Path
from tempfile import TemporaryDirectory
import json
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from fix_opengl_sdk_mirror import SOURCES, switch_source


def run():
    with TemporaryDirectory() as root:
        folder = Path(root)
        (folder / "tools").mkdir()
        target = folder / "tools" / "deps.json"
        sample = {"items": [
            {"name": "other", "url": "https://example.invalid/other.tar.gz", "sha256": "foo"},
        ]}
        for name, spec in SOURCES.items():
            sample["items"].append({
                "name": name, "kind": spec["kind"], "url": spec["original"],
                spec["pin_key"]: spec["pin"],
            })
        target.write_text(json.dumps(sample), encoding="utf-8")
        assert "redirected" in switch_source(folder)
        result = json.loads(target.read_text())
        for name, spec in SOURCES.items():
            entry = next(x for x in result["items"] if x["name"] == name)
            assert entry["url"] == spec["mirror"]
            assert entry[spec["pin_key"]] == spec["pin"]
        assert result["items"][0] == sample["items"][0]
        assert "already pinned" in switch_source(folder)

        # A change to any of the three pins must fail closed, with no rewrite.
        for bad_name, bad_spec in SOURCES.items():
            bad = json.loads(json.dumps(sample))
            item = next(x for x in bad["items"] if x["name"] == bad_name)
            item[bad_spec["pin_key"]] = "0" * len(bad_spec["pin"])
            target.write_text(json.dumps(bad), encoding="utf-8")
            try:
                switch_source(folder)
            except ValueError as exc:
                assert bad_name in str(exc)
            else:
                raise AssertionError(f"Changed pin accepted: {bad_name}")
            assert json.loads(target.read_text()) == bad, "Fail-closed mutated the manifest"
    print("PASS: all three exact-pin mirrors, idempotency and fail-closed guards")


if __name__ == "__main__":
    run()
