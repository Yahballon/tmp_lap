#!/usr/bin/env python3
"""Add PS5 selection music to the pinned WKAL v0.4.0 GAME installer.

Run after patch_game.py and after producing assets/snd0.at9 with the
ps5-at9-converter (--param-json assets/param.json.template).
"""
import argparse
import json
from pathlib import Path


def splice_once(code, old, new, what):
    if code.count(old) != 1:
        raise RuntimeError(f'Unexpected upstream installer structure for {what}: {code.count(old)} matches')
    return code.replace(old, new, 1)


def patch(root: Path):
    app = root / 'src/app_installer.c'
    data = root / 'assets/snd0.at9'
    param = root / 'assets/param.json.template'
    if not all(path.is_file() for path in (app, data, param)):
        raise FileNotFoundError('Expected src/app_installer.c, assets/snd0.at9, assets/param.json.template')
    song = data.read_bytes()
    if len(song) > 2097152 or len(song) < 168 or not song.startswith(b'RIFF') or song[8:12] != b'WAVE':
        raise ValueError('snd0.at9 is not a valid-sized RIFF/WAVE (ATRAC9) file')
    meta = json.loads(param.read_text())
    if meta.get('titleId') != 'WKAG00001' or meta.get('applicationCategoryType') != 0:
        raise ValueError('Expected GAME patch first, refusing to modify a Media launcher')
    if not isinstance(meta.get('pubtools', {}).get('loudnessSnd0'), str):
        raise ValueError('Missing pubtools.loudnessSnd0; run AT9 converter with --param-json')

    code = app.read_text(encoding='utf-8')
    if 'INCASSET(snd0_at9,' in code:
        raise ValueError('This source already embeds snd0.at9')
    code = splice_once(code,
        'INCASSET(icon0_png, "assets/icon0.png");',
        'INCASSET(icon0_png, "assets/icon0.png");\nINCASSET(snd0_at9, "assets/snd0.at9");',
        'embedded selection audio')
    code = splice_once(code,
        '  char icon_path[256];',
        '  char icon_path[256];\n  char sound_path[256];',
        'audio path buffer')
    code = splice_once(code,
        '  snprintf(icon_path, sizeof(icon_path), "/user/app/%s/sce_sys/icon0.png",\n           title_id);',
        '  snprintf(icon_path, sizeof(icon_path), "/user/app/%s/sce_sys/icon0.png",\n           title_id);\n'
        '  snprintf(sound_path, sizeof(sound_path), "/user/app/%s/sce_sys/snd0.at9",\n           title_id);',
        'audio path')
    code = splice_once(code,
        '    if (needs_update(icon_path, icon0_png, icon0_png_size))\n      update_needed = 1;',
        '    if (needs_update(icon_path, icon0_png, icon0_png_size))\n      update_needed = 1;\n'
        '    if (needs_update(sound_path, snd0_at9, snd0_at9_size))\n      update_needed = 1;',
        'audio update checks')
    code = splice_once(code,
        '  if ((err = install_app(title_id, "/user/app/"))) {',
        '  if (install_file(sound_path, snd0_at9, snd0_at9_size)) {\n'
        '    wkali_log("[WKALI] Failed to install snd0.at9\\n");\n'
        '    sceAppInstUtilTerminate();\n'
        '    return -1;\n'
        '  }\n\n'
        '  if ((err = install_app(title_id, "/user/app/"))) {',
        'audio install call')
    app.write_text(code, encoding='utf-8')
    print(f'Added PS5 Game tile selection audio ({len(song):,} bytes)')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('upstream', type=Path)
    patch(parser.parse_args().upstream)
