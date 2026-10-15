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

release_workflows = {}
for meta in sorted((root / "homebrew").glob("*/app.json")):
    data = json.loads(meta.read_text())
    slug = meta.parent.name
    assert data["slug"] == slug, (meta, data["slug"])
    matching = []
    for workflow in workflow_dir.glob("build-*.yml"):
        code = workflow.read_text()
        if f"homebrew/{slug}/" not in code or "gh release create" not in code:
            continue
        heading = re.search(r"(?m)^name:\s*(.+?)\s*$", code)
        assert heading, f"Missing 'name' in {workflow}"
        matching.append((heading.group(1).strip("'\""), workflow.name))
    if not matching:
        # Some scaffold or experimental apps intentionally have no release workflow.
        continue
    release_workflows[slug] = matching
    assert any(name in names for name, _ in matching), (
        f"{slug} is published by {matching} but no matching workflow_run "
        f"trigger is listed in update-version-catalog.yml"
    )

for slug in ("audiobook-reader", "ps5-system-info", "crypto-tracker", "payload-launcher"):
    assert slug in release_workflows, f"Expected release workflow missing: {slug}"

print("PASS: TESTBUILDS catalog tracks all", len(release_workflows),
      "discovered release-producing apps by exact workflow name")
