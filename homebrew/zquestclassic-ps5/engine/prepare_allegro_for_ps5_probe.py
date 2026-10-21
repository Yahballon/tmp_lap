#!/usr/bin/env python3
"""PS5-only Allegro CMake diagnostic: test desktop GLU dependency.

Do not stub any OpenGL/GLU symbols or claim successful graphics. The pinned
Allegro fork blocks its SDL backend unless BOTH desktop OpenGL and GLU are
reported. PS5 OpenGL exists, but ps5-opengl has no GLU. Relax exactly this
CMake configure condition to discover whether real GLU symbols are needed
by compile/link stages, where missing functions must still fail.
The upstream source is staged and remains pinned; runtime code unchanged.
"""
from pathlib import Path
import argparse
p=argparse.ArgumentParser()
p.add_argument("allegro_root",type=Path)
args=p.parse_args()
path=args.allegro_root/"CMakeLists.txt"
text=path.read_text()
condition="if(OPENGL_FOUND AND OPENGL_GLU_FOUND)"
new_condition="if(OPENGL_FOUND AND (OPENGL_GLU_FOUND OR ZC_PS5_NO_GLU_PROBE))"
assert text.count(condition)==1,"Pinned Allegro OpenGL guard changed"
assert 'message(FATAL_ERROR "SDL support currently requires OpenGL or OpenGL ES support.")' in text
assert 'if(OPENGL_opengl_LIBRARY)' in text
text=text.replace(condition,new_condition,1)
start_marker="install(\n  TARGETS ${ALLEGRO_TARGETS}\n  EXPORT ${PKG_TARGETS_NAME}\n)"
end_marker="#-----------------------------------------------------------------------------#"
assert text.count(start_marker)==1,"Allegro install/export block changed"
start=text.index(start_marker)
end=text.index(end_marker,start)
assert "export(EXPORT ${PKG_TARGETS_NAME}" in text[start:end]
assert "install(\n  EXPORT ${PKG_TARGETS_NAME}" in text[start:end]
text=text[:start]+"# PS5 probe: omit desktop package exports for SDL2-static.\n"+text[end:]
path.write_text(text)

# The PS5 libc exposes POSIX strerror_r, not the Annex-K strerror_s.
# This Allegro file's generated flags incorrectly enable both; only disable
# this unreachable error-reporting fallback in the staged PS5 C source.
stdio=args.allegro_root/"src/file_stdio.c"
source=stdio.read_text()
guard="#if defined(ALLEGRO_HAVE_STRERROR_S)"
assert source.count(guard)==1,"Pinned Allegro libc guard changed"
assert "strerror_r(userdata->errnum" in source
assert "strerror_s(userdata->errmsg" in source
source=source.replace(guard,"#if defined(ALLEGRO_HAVE_STRERROR_S) && !defined(ZC_PS5_LIBC_NO_STRERROR_S)",1)
stdio.write_text(source)

# The pinned extensions.c includes GLU unconditionally for desktop OpenGL,
# but uses no GLU functions (only commented-out diagnostic gluGetString).
# PS5 core OpenGL has no GLU header. Skip this unused header in PS5 build,
# without inventing a GLU implementation.
ext=args.allegro_root/"src/opengl/extensions.c"
ext_source=ext.read_text()
glu_guard="#elif !defined ALLEGRO_CFG_OPENGLES"
assert ext_source.count(glu_guard)==1,"Upstream GLU preprocessor guard changed"
assert ext_source.count("#include <GL/glu.h>")==1
assert "gluGetString(GLU_VERSION);" in ext_source  # commented-out upstream use
ext_source=ext_source.replace(glu_guard,
    "#elif !defined ALLEGRO_CFG_OPENGLES && !defined(ZC_PS5_NO_GLU_HEADER)",1)
ext.write_text(ext_source)
print("PS5 Allegro: bypass unused GLU header and missing strerror_s, no desktop exports; no shims")
