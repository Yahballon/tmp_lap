#!/usr/bin/env python3
"""Static PS5NESX Mesen-HD-pack plumbing/opt-in regression checks."""
from pathlib import Path
import json
root=Path(__file__).resolve().parents[1]
app=json.loads((root/'app.json').read_text())
assert app['version']=='1.0.17'
assert app['hdPacks']['enabledByDefault'] is False
mk=(root/'nes/tools/core-sources.mk').read_text()
assert 'HAVE_HDPACK := 1' in mk
build=(root/'nes/tools/build_shelf.py').read_text()
assert "'-DHAVE_HDPACK'" in build
core=(root/'nes/src/core_session.cpp').read_text()
assert 'm_system_directory.c_str()' in core
assert 'm_directory.c_str()' in core
assert 'w > 2560 || h > 2400' in core
src=(root/'nes/src/shelf_runtime.cpp').read_text()
assert 'get("NES/HDPacks","false")' in src
assert 'core.SetOption("fceumm_hdpacks",' in src
assert 'root+"/HdPacks/"+stem+"/hires.txt"' in src
assert 'const float scale=fit>=1.0f?' in src
settings=(root/'ps5/frontend/fe_options.cpp').read_text()
assert 'Toggle("NES/HDPacks", "HD texture packs", "false"' in settings
main=(root/'nes/src/main_shelf.cpp').read_text()
assert '"HdPacks"' in main
print('PASS PS5NESX experimental HD pack paths, build flags, opt-in and video bounds')
