#!/usr/bin/env python3
"""Regression: dependency substitutions may never change the expected bytes."""
from pathlib import Path
from tempfile import TemporaryDirectory
import json
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from fix_opengl_sdk_mirror import (SOURCES, UI_KIT_OLD, UI_KIT_NEW, UI_KIT_SHA256, switch_source)


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
        sample["items"].append({"name": "ui-kit", "kind": "files", "files": [
            {"name": name, "url": UI_KIT_OLD + name, "sha256": pin}
            for name, pin in UI_KIT_SHA256.items()
        ]})
        target.write_text(json.dumps(sample), encoding="utf-8")
        assert "redirected" in switch_source(folder)
        result = json.loads(target.read_text())
        for name, spec in SOURCES.items():
            entry = next(x for x in result["items"] if x["name"] == name)
            assert entry["url"] == spec["mirror"]
            assert entry[spec["pin_key"]] == spec["pin"]
        assert result["items"][0] == sample["items"][0]
        kit = next(x for x in result["items"] if x["name"] == "ui-kit")
        assert len(kit["files"]) == 10
        for item in kit["files"]:
            assert item["url"] == UI_KIT_NEW + item["name"]
            assert item["sha256"] == UI_KIT_SHA256[item["name"]]
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
        bad = json.loads(json.dumps(sample))
        kit = next(x for x in bad["items"] if x["name"] == "ui-kit")
        kit["files"][0]["sha256"] = "0" * 64
        target.write_text(json.dumps(bad), encoding="utf-8")
        try:
            switch_source(folder)
        except ValueError as exc:
            assert "ui-kit pin changed" in str(exc)
        else:
            raise AssertionError("Changed UI-kit hash unexpectedly accepted")
        assert json.loads(target.read_text()) == bad
    print("PASS: 3 exact dependency pins + 10 UI-kit file hashes, idempotency, fail-closed guards")


if __name__ == "__main__":
    run()
