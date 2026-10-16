#!/usr/bin/env python3
"""Build isolated PS5MesenX frontend sources from pinned PS5SNESX.

Only source preparation: does NOT produce a signed PS5 executable.
"""
from pathlib import Path
import json
import shutil

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = ROOT / "upstream/PS5SNESX"
OUTPUT = ROOT / "build/port-src"
TITLE_ID = "PPSA99896"
TITLE = "PS5MesenX"

def replace_required(path, old, new):
    contents = path.read_text()
    if old not in contents:
        raise RuntimeError(f"Missing required upstream patch point: {path.name}: {old[:80]!r}")
    path.write_text(contents.replace(old, new))

def generate():
    if OUTPUT.exists():
        shutil.rmtree(OUTPUT)
    shutil.copytree(ORIGINAL, OUTPUT, symlinks=True)
    # The pinned PS5SNESX revision predates build-mesa-util.sh. Vendor the
    # independently licensed helper from upstream and fail early if absent.
    mesa_helper = ROOT / "vendor/build-mesa-util.sh"
    if not mesa_helper.is_file():
        raise RuntimeError("Missing PS5 Mesa utility build helper")
    helper_target = OUTPUT / "ps5/tools/build-mesa-util.sh"
    helper_target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(mesa_helper, helper_target)
    helper_target.chmod(0o755)
    source = OUTPUT / "snes/src"
    for name in ("core_session.h", "core_session.cpp"):
        shutil.copy2(ROOT / "src" / ("libretro_" + name), source / name)

    # Correct game detection: SNES-only validation must never reject iNES.
    rom = source / "games.cpp"
    replace_required(rom,
        'return ext==".sfc"||ext==".smc"||ext==".swc"||ext==".fig";',
        'return ext==".nes";')
    replace_required(rom, "ZIP contains no SNES ROM", "ZIP contains no NES ROM")
    before = rom.read_text()
    first = before.index('    // (AI-assisted) Validate normal SNES cartridge headers')
    last = before.index('    error.clear();return true;', first)
    after = '''    // Accept iNES/NES2.0 header only; ignore unrelated SNES cart heuristics.
    if (rom.size() < 16 || rom.size() > (64u << 20) ||
        std::memcmp(rom.data(), "NES\\x1A", 4) != 0) {
        error = "Not a valid iNES/NES2 ROM"; rom.clear(); return false;
    }
'''
    rom.write_text(before[:first] + after + before[last:])

    shelf = source / "shelf_games.cpp"
    replace_required(shelf, 's==".sfc"||s==".smc"||s==".swc"||s==".fig"||s==".zip"',
                     's==".nes"||s==".zip"')
    replace_required(shelf, 'SNES-%08X', 'MESEN-%08X')
    replace_required(shelf, 'SNES ROM (FCEUmm)', 'NES ROM (MesenCE)')
    replace_required(shelf, 'game.badges={"SNES"};', 'game.badges={"NES"};')

    runtime = source / "shelf_runtime.cpp"
    replace_required(runtime, '''    core.SetOption("bsnes_region",get("SNES/Region","auto"));
    core.SetOption("bsnes_violate_accuracy","enabled");
    core.SetOption("bsnes_chip_hle",get("SNES/ChipMode","HLE"));
    core.SetOption("bsnes_gamma_ramp",get("SNES/Gamma","disabled"));''',
        '''    core.SetOption("mesen_hd_packs",get("NES/HdPacks","enabled"));''')
    replace_required(runtime, 'get("SNES/Aspect","4:3")', 'get("NES/Aspect","4:3")')
    replace_required(runtime, 'get("SNES/Sound","true")', 'get("NES/Sound","true")')
    replace_required(runtime, '"SNES audio port failed"', '"MesenCE audio port failed"')
    replace_required(runtime, '"SNES frame upload failed"', '"MesenCE frame upload failed"')

    # Home directory also owns MesenCE HdPacks/<ROM stem>/hires.txt.
    main = source / "main_shelf.cpp"
    replace_required(main, 'PS5SNES', TITLE)
    replace_required(main, 'PPSA99205', TITLE_ID)
    replace_required(main, '"/SNES"', '"/NES"')
    # Share ONLY the ROM library with PS5NES. Keep saves, cover cache,
    # HD packs and logs in /data/PS5MesenX to avoid clobbering other apps.
    replace_required(main, 'paths.games_dir=root+"/games";',
                     'paths.games_dir="/data/PS5NES/games";')
    replace_required(main,
                     '    OrbisFrontendPaths paths;paths.top_dir=root;',
                     '''    // A shared ROM directory must exist even when PS5NES is not installed.
    if (mkdir("/data/PS5NES",0777)!=0 && errno!=EEXIST)
        std::printf("[games] could not create /data/PS5NES (errno=%d)\\n",errno);
    if (mkdir("/data/PS5NES/games",0777)!=0 && errno!=EEXIST)
        std::printf("[games] could not create /data/PS5NES/games (errno=%d)\\n",errno);
    std::printf("[games] shared NES ROM directory: /data/PS5NES/games\\n");
    OrbisFrontendPaths paths;paths.top_dir=root;''')
    # Also replace the old games folder in the supplemental scan list,
    # or the frontend could show duplicate ROM entries.
    replace_required(main, '"/data/PS5MesenX/games","/data/PS5MesenX/roms"',
                     '"/data/PS5MesenX/roms"')

    replace_required(main, 'for(const char* dir:{"games","saves","settings","covers","cache","logs"})',
                    'for(const char* dir:{"games","saves","settings","covers","cache","logs","HdPacks","SaveStates","Saves"})')

    # The SNES frontend cache paths and per-ROM NFS root must be independent.
    replace_required(shelf, 'PS5SNES', TITLE)
    for name in ("INSTALL.md",):
        f = OUTPUT / "snes" / name
        if f.exists():
            f.write_text(f.read_text().replace("PS5SNES", TITLE).replace("PPSA99205", TITLE_ID))

    param_path = OUTPUT / "snes/sce_sys/param.json"
    param = json.loads(param_path.read_text())
    param["titleId"] = TITLE_ID
    param["conceptId"] = TITLE_ID.replace("PPSA", "")
    param["contentId"] = f"UP9000-{TITLE_ID}_00-PS5MESENX0000001"
    param["localizedParameters"]["en-US"]["titleName"] = TITLE
    param["contentVersion"] = "01.000.002"
    param_path.write_text(json.dumps(param, indent=2) + "\n")
    for file in (rom, shelf, runtime, main):
        if "PS5SNES" in file.read_text() or "bsnes_" in file.read_text():
            raise RuntimeError(f"Legacy SNES app or core identifier in {file}")
    assert "NES\\x1A" in rom.read_text()
    # Fail the build early if the shared NES path is not actually wired.
    compiled_main=main.read_text()
    assert 'paths.games_dir="/data/PS5NES/games";' in compiled_main
    assert 'mkdir("/data/PS5NES/games",0777)' in compiled_main
    assert '"/data/PS5MesenX/games"' not in compiled_main
    print("Prepared PS5MesenX source under", OUTPUT)
    print("NOTE: Core build/link not implemented in this preparation script.")

if __name__ == "__main__":
    generate()
