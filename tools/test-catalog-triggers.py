#!/usr/bin/env python3
"""Prevent silent TESTBUILDS version.json drift when a new app build is added.

Only successful main-branch release builds should refresh the catalog.
GitHub Actions release events generated with GITHUB_TOKEN do not fire other
workflows, so our catalog must subscribe to the EXACT build workflow names.
"""
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
workflow_dir = root / ".github" / "workflows"
catalog = (workflow_dir / "update-version-catalog.yml").read_text()
match = re.search(r"(?ms)^  workflow_run:\n(.*?)(?=^\S|\Z)", catalog)
assert match, "Catalog has no workflow_run trigger"
block = match.group(1)
names = set(re.findall(r"(?m)^      - (.+?)\s*$", block))
assert names, "No build workflows subscribed by catalog updater"
assert "types: [completed]" in block, "Catalog must run after builds complete"
assert "Publish PS5DreamcastX Release" in names, "DreamcastX standalone release publisher not subscribed"
assert "Publish PS5DreamcastX TESTBUILD" in names

release_workflows = {}
# Legacy Crypto Tracker has no app.json; still require its release workflow.
app_dirs = {m.parent for m in (root / "homebrew").glob("*/app.json")}
app_dirs.add(root / "homebrew" / "crypto-tracker")
for app_dir in sorted(app_dirs):
    slug = app_dir.name
    meta = app_dir / "app.json"
    if meta.exists():
        data = json.loads(meta.read_text())
        assert data.get("slug", slug) == slug, (meta, data.get("slug"))
    matching = []
    for workflow in workflow_dir.glob("build-*.yml"):
        code = workflow.read_text()
        if f"homebrew/{slug}/" not in code or "gh release create" not in code:
            continue
        heading = re.search(r"(?m)^name:\s*(.+?)\s*$", code)
        assert heading, f"Missing 'name' in {workflow}"
        matching.append((heading.group(1).strip("'\""), workflow.name))
    if slug == "ps5-dreamcastx":
        # DreamcastX deliberately separates its expensive native build from
        # GitHub release publishing. The publisher only takes successful,
        # already-verified PS5 artifacts and must be subscribed by catalog CI.
        publisher = workflow_dir / "publish-ps5-dreamcastx.yml"
        pub_code = publisher.read_text()
        assert "gh release create" in pub_code
        assert "PS5_RELEASE_TOKEN" in pub_code
        testbuild = workflow_dir / "publish-ps5-dreamcastx-testbuild.yml"
        testbuild_code = testbuild.read_text()
        assert "Build PS5DreamcastX (native Flycast)" in testbuild_code
        assert "Publish PS5DreamcastX TESTBUILD" in names
        assert "Publish PS5DreamcastX Release" in names
        matching.append(("Publish PS5DreamcastX Release", publisher.name))
    if not matching:
        # Some scaffold or experimental apps intentionally have no release workflow.
        continue
    release_workflows[slug] = matching
    assert any(name in names for name, _ in matching), (
        f"{slug} is published by {matching} but no matching workflow_run "
        f"trigger is listed in update-version-catalog.yml"
    )

for slug in ("audiobook-reader", "ps5-system-info", "crypto-tracker", "payload-launcher", "ps5-dreamcastx"):
    assert slug in release_workflows, f"Expected release workflow missing: {slug}"

print("PASS: TESTBUILDS catalog tracks all", len(release_workflows),
      "discovered release-producing apps by exact workflow name")
