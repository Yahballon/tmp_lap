#!/usr/bin/env python3
"""Package a DISTINCT PS5 title for on-console original zplayer boot diagnostics.

This NEVER changes public PPSA99127 or TESTBUILDS version.json.
Source FSELF must be from an already-successful original zplayer cross-build;
this script does not recompile or claim the engine has run successfully.
"""
import argparse
import hashlib
import json
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED

TITLE_ID = "PPSA99128"
CONTENT_ID = "UP9000-PPSA99128_00-ZQUESTENGINEBOOT"
assert len(CONTENT_ID.split("-")[-1]) == 16
PROVENANCE = "ZQuestClassic/ZQuestClassic@c44dde7d0146da78bf8b541af457d5ff4e67a830"

p = argparse.ArgumentParser()
p.add_argument("--fself", required=True, type=Path)
p.add_argument("--libc", required=True, type=Path)
p.add_argument("--param-template", required=True, type=Path)
p.add_argument("--icon", required=True, type=Path)
p.add_argument("--source-run", required=True, type=str)
p.add_argument("--output", required=True, type=Path)
a = p.parse_args()

if not a.source_run.isdigit() or len(a.source_run) < 8:
    raise SystemExit("Invalid source GitHub Actions run ID")
for source in (a.fself, a.libc, a.param_template, a.icon):
    if not source.is_file() or not source.stat().st_size:
        raise SystemExit(f"Missing nonempty original input: {source}")
if a.fself.stat().st_size < 100_000:
    raise SystemExit("Original zplayer FSELF unexpectedly small")
if a.icon.read_bytes()[:8] != b"\x89PNG\r\n\x1a\n":
    raise SystemExit("Invalid icon PNG header")

param = json.loads(a.param_template.read_text())
param.update(
    titleId=TITLE_ID,
    conceptId=TITLE_ID[4:],
    contentId=CONTENT_ID,
    contentVersion="01.000.001",
)
assert "localizedParameters" in param
assert "en-US" in param["localizedParameters"]
param["localizedParameters"]["en-US"]["titleName"] = "ZQuest ENGINE BOOT TEST"
if "defaultLanguage" in param and param["defaultLanguage"] != "en-US":
    # Preserve original template's locale structure.
    pass
assert param["titleId"] != "PPSA99127"

sha = hashlib.sha256(a.fself.read_bytes()).hexdigest()
manifest = {
    "title_id": TITLE_ID,
    "test_kind": "original-zplayer-boot-only",
    "source_run_id": a.source_run,
    "upstream_source": PROVENANCE,
    "eboot_sha256": sha,
    "gameplay_verified": False,
    "ps5_boot_verified": False,
    "does_not_replace": "PPSA99127",
    "log_paths": [
        "/data/ZQuestEngineBoot/logs/boot.log",
        "/data/ZQuestEngineBoot/logs/crash.log",
        "/data/ZQuestClassic/logs/engine-boot.log",
    ],
}
instructions = """ZQuest Classic PS5 — ORIGINAL ZPLAYER BOOT DIAGNOSTIC
========================================================
This package is EXPERIMENTAL and may crash, freeze or exit immediately.
It is NOT a playable release and may lack original runtime resources.
The app's Title-ID PPSA99128 is separate from the working PPSA99127.

On a jailbroken PS5 with suitable homebrew loader:
  1. Install/launch PPSA99128 (ZQuest ENGINE BOOT TEST).
  2. Check /data/ZQuestEngineBoot/logs/boot.log and crash.log via FTP.
  3. If absent check /data/ZQuestClassic/logs/engine-boot.log.
  4. Send the log text or a screenshot of the actual console behavior.

Do NOT overwrite PPSA99127's tested v0.1.5 app.
Do NOT interpret a startup banner, FSELF signing or a blank screen as
proof of Quest loading or gameplay. No Nintendo/quest media is bundled.
Original engine uses GPLv3; provide corresponding source to recipients.
"""
a.output.parent.mkdir(parents=True, exist_ok=True)
with ZipFile(a.output, "w", ZIP_DEFLATED, compresslevel=6) as z:
    root = TITLE_ID + "/"
    z.write(a.fself, root + "eboot.bin")
    z.write(a.libc, root + "sce_module/libc.prx")
    z.write(a.icon, root + "sce_sys/icon0.png")
    z.writestr(root + "sce_sys/param.json", json.dumps(param, indent=2) + "\n")
    z.writestr("ENGINE_BOOT_TEST_README.txt", instructions)
    z.writestr("ENGINE_BOOT_TEST_PROVENANCE.json", json.dumps(manifest, indent=2) + "\n")
with ZipFile(a.output) as z:
    expected = {TITLE_ID + "/" + x for x in (
        "eboot.bin", "sce_module/libc.prx",
        "sce_sys/icon0.png", "sce_sys/param.json",
    )}
    assert expected.issubset(z.namelist()), sorted(expected - set(z.namelist()))
    assert all(".." not in Path(s).parts for s in z.namelist())
    params = json.loads(z.read(TITLE_ID + "/sce_sys/param.json"))
    assert params["titleId"] == TITLE_ID and params["contentId"] == CONTENT_ID
    assert params["titleId"] != "PPSA99127"
    assert hashlib.sha256(z.read(TITLE_ID + "/eboot.bin")).hexdigest() == sha
print(f"ENGINE_BOOT_DIAGNOSTIC_ZIP={a.output}")
print(f"FSELF_SHA256={sha}")
print("PLAYABLE=NO; BOOT_VERIFIED_ON_HARDWARE=NO; PUBLIC_TESTBUILDS_UNCHANGED=YES")
