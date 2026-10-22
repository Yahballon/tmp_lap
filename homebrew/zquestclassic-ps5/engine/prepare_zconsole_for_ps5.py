#!/usr/bin/env python3
"""Native PS5 logger port: redirect desktop text-window output to stderr.

The upstream logger implements non-Windows console output exclusively via
Allegro's native desktop text-dialog addon. That GUI is absent on PS5.
Instead of fabricating Allegro's al_open_native_text_log functions, keep all
upstream formatting and write the actual log text to the native stderr stream.
Only the PS5 diagnostic build is modified, not desktop builds.
"""
from pathlib import Path
import argparse
import re

p = argparse.ArgumentParser()
p.add_argument("upstream_root", type=Path)
a = p.parse_args()
cpp = a.upstream_root / "src/zconsole/ConsoleLogger.cpp"
s = cpp.read_text()
opening = '#ifdef __EMSCRIPTEN__\n\treturn 0;\n#else\n\tif (m_textlog) {'
assert s.count(opening) == 1, "Original ConsoleLogger::Create changed"
s = s.replace(opening,
    '#if defined(ZC_PS5_NO_NATIVE_TEXTLOG)\n\treturn 0;\n'
    '#elif defined(__EMSCRIPTEN__)\n\treturn 0;\n'
    '#else\n\tif (m_textlog) {', 1)

closing = '#ifndef __EMSCRIPTEN__\n\tif (m_textlog) {\n\t\tal_close_native_text_log(m_textlog);'
assert s.count(closing) == 1, "Original ConsoleLogger::kill changed"
s = s.replace(closing,
    '#if !defined(__EMSCRIPTEN__) && !defined(ZC_PS5_NO_NATIVE_TEXTLOG)\n'
    '\tif (m_textlog) {\n\t\tal_close_native_text_log(m_textlog);', 1)

valid = '#ifdef __EMSCRIPTEN__\n\treturn 0;\n#else\n\treturn m_textlog != NULL;'
assert s.count(valid) == 1, "Original ConsoleLogger::valid changed"
s = s.replace(valid,
    '#if defined(ZC_PS5_NO_NATIVE_TEXTLOG)\n\treturn true;\n'
    '#elif defined(__EMSCRIPTEN__)\n\treturn 0;\n'
    '#else\n\treturn m_textlog != NULL;', 1)

pattern = re.compile(
    r'#ifndef __EMSCRIPTEN__\n'
    r'(\s*)if \(m_textlog\) al_append_native_text_log'
    r'\(m_textlog, "%s", (\w+)\);\n'
    r'#endif'
)
hits = pattern.findall(s)
assert len(hits) == 6, "Original formatted logger output sites changed"
def emit(match: re.Match[str]) -> str:
    indent, variable = match.group(1), match.group(2)
    return (
        '#if defined(ZC_PS5_NO_NATIVE_TEXTLOG)\n'
        f'{indent}fputs({variable}, stderr);\n'
        f'{indent}fflush(stderr);\n'
        '#elif !defined(__EMSCRIPTEN__)\n'
        f'{indent}if (m_textlog) al_append_native_text_log(m_textlog, "%s", {variable});\n'
        '#endif'
    )
s = pattern.sub(emit, s)
assert s.count("al_append_native_text_log(") == 6
cpp.write_text(s)

# Apply only to the logger library. Do not globally spoof Emscripten or
# disable game input, ZScript VM, quest loading or audio code.
cmake = a.upstream_root / "CMakeLists.txt"
c = cmake.read_text()
anchor = 'target_link_libraries(zconsolelogger PUBLIC allegro_with_legacy base)'
assert c.count(anchor) == 1, "Original logger library definition changed"
c = c.replace(anchor, anchor + '\n'
    'if(ZC_PS5_TARGET_ONLY_PROBE)\n'
    '  target_compile_definitions(zconsolelogger PRIVATE ZC_PS5_NO_NATIVE_TEXTLOG)\n'
    'endif()', 1)
cmake.write_text(c)
print("PS5 ConsoleLogger uses real stderr output without desktop Allegro text windows")
