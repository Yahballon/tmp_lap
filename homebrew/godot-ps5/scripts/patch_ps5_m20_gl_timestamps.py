#!/usr/bin/env python3
"""M20: avoid synchronous GPU timestamp readbacks in Godot's PS5 GLES3 runtime.

The upstream Godot 4.7.2 GLES3 Utilities::_capture_timestamps_begin() calls
glGetQueryObjectui64v(..., GL_QUERY_RESULT, ...). That is an inherently
blocking read of the GPU timeline. M19 real-hardware timings show unresolved
seconds-long stalls inside RenderingServer::draw, outside 23 timed GL paths.
This controlled candidate compiles out *only* GPU timestamp query issuance and
retrieval on the experimental PS5 GLES3 target. Godot's CPU timing, game
rendering, physics and draw submissions remain intact.
"""
from pathlib import Path
import sys

MARKER = "GODOT_PS5_M20_GPU_TIMESTAMPS_DISABLED"
BEGIN = "#if !(defined(PS5_ENABLED) && defined(GODOT_PS5_NATIVE_GLES3))"
END = "#endif // GODOT_PS5_M20_GPU_TIMESTAMPS_DISABLED"

def patch(source: str) -> str:
    if MARKER in source:
        if source.count(MARKER) != 2:
            raise ValueError("Partially modified GPU timestamp instrumentation")
        return source
    anchor1 = """\t\tglQueryCounter(frames[frame].queries[frames[frame].timestamp_count], GL_TIMESTAMP);"""
    anchor2 = """\t\t\t\tglGetQueryObjectui64v(frames[frame].queries[i], GL_QUERY_RESULT, &temp);"""
    if source.count(anchor1) != 1 or source.count(anchor2) != 1:
        raise ValueError("Unexpected upstream Godot 4.7.2 GL timestamp anchors")
    # Guard the whole GL_API_ENABLED code in each function, preserving all
    # surrounding CPU timestamps and the original behavior on non-PS5 targets.
    source = source.replace(anchor1, BEGIN + "\n" + anchor1 + "\n" + END, 1)
    source = source.replace(anchor2, BEGIN + "\n" + anchor2 + "\n" + END, 1)
    if source.count(MARKER) != 2:
        raise AssertionError("PS5 timestamp compile guards incomplete")
    return source

def main():
    target = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("drivers/gles3/storage/utilities.cpp")
    old = target.read_text()
    new = patch(old)
    if old != new:
        target.write_text(new)
        print("GODOT_PS5_M20_GPU_TIMESTAMP_READBACK_DISABLED")
    else:
        print("GODOT_PS5_M20_GPU_TIMESTAMP_PATCH_ALREADY_APPLIED")

if __name__ == "__main__":
    main()
