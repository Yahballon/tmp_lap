#!/usr/bin/env python3
"""Audit linked upstream zplayer as a PS5 app input, not a playable package.

The raw CMake cross-link is valuable but not yet a FSELF. A native PS5
application needs an initialized CRT _start, correct PIE layout/imports,
Sony module conversion, and on-console validation. This explicitly reports
missing preconditions rather than treating a successful link as launchability.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess

p = argparse.ArgumentParser()
p.add_argument("binary", type=Path)
p.add_argument("--report", type=Path, required=True)
p.add_argument("--readelf", default="llvm-readelf-18")
args = p.parse_args()

if not args.binary.is_file() or args.binary.stat().st_size == 0:
    raise SystemExit("Missing linked ZQuestClassic engine ELF")

result = subprocess.run([args.readelf, "-h", str(args.binary)],
                        check=True, capture_output=True, text=True)
header = result.stdout
def read_field(label: str) -> str:
    match = re.search(r"^\\s*" + re.escape(label) + r":\\s*(.+)$",
                      header, re.MULTILINE)
    if not match:
        raise SystemExit(f"Missing ELF header field: {label}")
    return match.group(1).strip()

entry = int(read_field("Entry point address"), 16)
machine = read_field("Machine")
kind = read_field("Type")
if "X86-64" not in machine and "x86-64" not in machine:
    raise SystemExit(f"Not a PS5 x86_64 cross-compiled ELF: {machine}")

# Do not imply an ELF with a nonzero entry is a valid PS5 module.
report = {
    "kind": "original-zplayer-cross-link-diagnostic",
    "full_upstream_engine_linked": True,
    "elf_type": kind,
    "machine": machine,
    "entry_point": f"0x{entry:x}",
    "has_nonzero_entry_point": entry != 0,
    "ps5_fself_packaged": False,
    "runtime_boot_verified_on_ps5": False,
    "gameplay_verified_on_ps5": False,
    "ready_for_testbuilds": False,
    "next_work": [
        "Integrate PS5-DevBridge CRT _start and proper main/argv handoff",
        "Link with native PS5 PIE layout and module import conversion",
        "Package legal original engine resources and writable save folders",
        "Test actual .qst gameplay, controller, audio and saves on PS5",
    ],
}
args.report.parent.mkdir(parents=True, exist_ok=True)
args.report.write_text(json.dumps(report, indent=2) + "\n")
if entry == 0:
    print("BOOT_CONTRACT_BLOCKED=NO__START entry=0x0")
else:
    print("BOOT_CONTRACT_PROBE=NONZERO_ENTRY_NOT_YET_FSELF")
print("PS5_PLAYABLE_RELEASE=NO")
