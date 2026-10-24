#!/usr/bin/env python3
"""M19: real Godot->EGL proc resolution and bounded GL-call profiler guards."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
s=(root/"native/gles3_canvas_main.cpp").read_text()
ops=[
 "glDrawElements","glDrawArrays","glDrawElementsInstanced",
 "glDrawArraysInstanced","glBufferData","glBufferSubData",
 "glMapBufferRange","glUnmapBuffer","glTexImage2D","glTexSubImage2D",
 "glFenceSync","glClientWaitSync","glFinish","glFlush",
 "glClear","glBlitFramebuffer","glReadPixels","glGetError",
 "glBindFramebuffer","glUseProgram","glBindBuffer",
 "glGetIntegerv","glGetUniformLocation",
]
assert len(set(ops))==23
for op in ops:
    assert f'"{op}"' in s, op
    assert f"m19_real_{op}" in s, op
    assert f"m19_trace_{op}" in s, op
    assert f'std::strcmp(name,"{op}")==0' in s, op
    assert f"m19_real_{op}(" in s, op
assert "std::atomic<unsigned long long> calls" in s
assert "spikes_80ms.fetch_add" in s
assert "GODOT_PS5_M19_GL_PROFILER_READY" in s
assert "GODOT_PS5_M19_GL_5S" in s
assert "GODOT_PS5_M19_GL_SUMMARY_5S" in s
assert 'm19_report(after);' in s
assert 'return original;' in s and "eglGetProcAddress(name)" in s
assert "PS5PresentMode perf_present_mode = PS5PresentMode::VSync;" in s
assert 'GODOT_PS5_M13_NATIVE_FRAME_PROFILER_READY' in s
assert not re.search(r"(^|\\n)\\s*glFinish\\(",s)
assert "glReadPixels(0," not in s
assert not re.search(r"(^|\\n)\\s*glDraw(Arrays|Elements)\\(",s)
print("GODOT_PS5_M19_GL_API_TIMING_PASSTHROUGH_GUARDS_PASS")
