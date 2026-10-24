#!/usr/bin/env python3
"""M23: trace omitted PS5 GL resource, shader and status functions."""
from pathlib import Path
import re
r=Path(__file__).resolve().parents[1]
src=(r/"native/gles3_canvas_main.cpp").read_text()
header=(r/"native/m23_gl_trace_extra.hpp").read_text()
stage=(r/"scripts/stage_native_link.py").read_text()
names=re.findall(r'^    \{"(gl\w+)"\},?$',header,re.MULTILINE)
assert len(names)==32 and len(set(names))==32,(len(names),names)
for n in names:
    assert f'm23_original_{n}' in header,n
    assert f'm23_trace_{n}' in header,n
    assert f'std::strcmp(name,"{n}")==0' in header,n
    assert f'PFN{n.upper()}PROC' in header,n
for x in ("glBindTexture","glBindVertexArray","glGetSynciv","glDeleteSync",
          "glGetProgramiv","glGetShaderiv","glTexParameteri","glActiveTexture",
          "glVertexAttribPointer","glGenBuffers","glDeleteBuffers"):
    assert x in names,x
assert 'm23_sample(' in header and 'std::atomic<unsigned long long>' in header
assert 'GODOT_PS5_M23_GL_5S' in header
assert 'GODOT_PS5_M23_GL_SUMMARY_5S' in header
assert 'GODOT_PS5_M23_GL_PROFILER_READY' in header
assert '#include "m23_gl_trace_extra.hpp"' in src
assert 'return m23_resolve(name,original);' in src
assert 'm23_report(after);' in src
assert 'GODOT_PS5_M23_EXTRA_GL_TIMING active=1 methods=32' in src
assert 'GODOT_PS5_M19_GL_SUMMARY_5S' in src
assert 'PS5PresentMode perf_present_mode = PS5PresentMode::VSync;' in src
assert 'extra_header=Path(entry).parent/"m23_gl_trace_extra.hpp"' in stage
assert 'shutil.copyfile(extra_header,target/"m23_gl_trace_extra.hpp")' in stage
assert 'GODOT_PS5_M22_BOUNDED_CANVAS_RING' not in src
print("GODOT_PS5_M23_32_EXTRA_GL_FUNCS_SOURCE_GATES_PASS")
