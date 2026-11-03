#!/usr/bin/env python3
"""Patch exact-pinned Hydroid into the AndroidFlow multi-APK launcher; GPL-3.0."""
import argparse
import json
import shutil
import subprocess
from pathlib import Path

HYDROID_SHA = "d8371afd75e3da6ab888d467d1e6bb00c32d2beb"
TITLE_ID = "PPSA99920"

def replace_one(code, old, new, file):
    if code.count(old) != 1:
        raise RuntimeError(f"Source anchor changed in {file}: {old[:70]!r}, count={code.count(old)}")
    return code.replace(old, new, 1)

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--source", type=Path, required=True)
    args = p.parse_args()
    root = Path(__file__).resolve().parents[1]
    src = args.source.resolve()
    actual = subprocess.check_output(["git", "-C", str(src), "rev-parse", "HEAD"], text=True).strip()
    if actual != HYDROID_SHA:
        p.error(f"expected Hydroid commit {HYDROID_SHA}, got {actual}")
    app_file = src / "src/app_main.c"
    build_file = src / "title/build-title.sh"
    param = src / "title/sce_sys/param.json"
    app = app_file.read_text()
    app = replace_one(app, '#include "game_file.h"',
                      '#include "game_file.h"\n#include "yah_coverflow.h"', app_file)
    app = replace_one(app,
        '    int native_game = apk_game || stat(GAME_LIB, &st) == 0;',
        '    int launcher_mode = stat(APP0 "/androidflow.flag", &st) == 0;\n'
        '    int native_game = launcher_mode || apk_game || stat(GAME_LIB, &st) == 0;', app_file)
    app = replace_one(app,
        "    if (apk_game) {\n        // the app's game: single_game.txt names it, its file is in /app0/game",
        "    if (launcher_mode) {\n"
        "        static lib_game chosen;\n"
        "        int picked = yah_coverflow_pick(&chosen);\n"
        "        if (picked < 0) {\n"
        "            say(0, \"AndroidFlow launcher exited (%d)\", picked);\n"
        "            return picked == -1 ? 0 : 1;\n"
        "        }\n"
        "        game_started = 1;\n"
        "        quick_menu_set_game(chosen.name, chosen.id, chosen.icon, chosen.icon_size);\n"
        "        free(chosen.icon);\n"
        "        say(1, \"AndroidFlow: launch %s\", chosen.name);\n"
        "        droid_startup_begin();\n"
        "        game_settings_load(chosen.dir);\n"
        "        game_data_prepare();\n"
        "        set_known_package(&chosen);\n"
        "        run_apk(chosen.path);\n"
        "        halt();\n"
        "    }\n"
        "    if (apk_game) {\n"
        "        // the app's game: single_game.txt names it, its file is in /app0/game", app_file)
    app_file.write_text(app)
    build = build_file.read_text()
    build = replace_one(build, "game_file net_shim",
                        "game_file yah_coverflow net_shim", build_file)
    build = replace_one(build, 'mkdir -p "$app/assets" "$app/game" && cp -r',
                        'mkdir -p "$app/assets" "$app/game" && printf "AndroidFlow v0.1.0\\n" > "$app/androidflow.flag" && cp -r', build_file)
    build_file.write_text(build)
    meta = json.loads(param.read_text())
    meta.update(titleId=TITLE_ID, conceptId=TITLE_ID[4:],
                contentId="UP9000-PPSA99920_00-ANDROIDFLOW00001",
                contentVersion="01.000.000")
    meta["localizedParameters"]["en-US"]["titleName"] = "AndroidFlow"
    param.write_text(json.dumps(meta, indent=2) + "\n")
    for name in ("yah_coverflow.c", "yah_coverflow.h", "yah_coverflow_math.h"):
        shutil.copyfile(root / "native" / name, src / "src" / name)
    subprocess.run(["python3", str(root / "assets/generate_icon.py"),
                    str(src / "title/sce_sys/icon0.png")], check=True)
    print(f"Patched Hydroid {actual} for AndroidFlow {TITLE_ID}")

if __name__ == "__main__":
    main()
