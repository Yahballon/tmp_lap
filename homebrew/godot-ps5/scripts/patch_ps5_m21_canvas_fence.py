#!/usr/bin/env python3
"""M21: replace blocking canvas fence status queries with a nonblocking probe.

Original pinned Godot 4.7.2 GLES3 RasterizerCanvasGLES3::canvas_render_items
checks its rotating 2D instance-buffer fence with glGetSynciv(...GL_SYNC_STATUS).
On experimental PS5 Mesa/AGC this call remains uninstrumented by M19 and can
conceivably wait for GPU progress. We avoid *that* status query only on PS5.
No buffer may be treated as ready when a fence is not complete.
"""
from pathlib import Path
import sys

TAG = "GODOT_PS5_M21_NONBLOCKING_CANVAS_FENCE"
ORIGINAL = """		GLint syncStatus;
		glGetSynciv(state.canvas_instance_data_buffers[state.current_data_buffer_index].fence, GL_SYNC_STATUS, 1, nullptr, &syncStatus);"""
PATCH = """		GLint syncStatus;
#if defined(PS5_ENABLED) && defined(GODOT_PS5_NATIVE_GLES3)
		// M21: timeout=0 MUST NOT wait for the GPU. GL_WAIT_FAILED is not
		// evidence the buffer is available: retain original query as fallback.
		const GLenum m21_poll = glClientWaitSync(state.canvas_instance_data_buffers[state.current_data_buffer_index].fence, 0, 0);
		if (m21_poll == GL_WAIT_FAILED) {
			glGetSynciv(state.canvas_instance_data_buffers[state.current_data_buffer_index].fence, GL_SYNC_STATUS, 1, nullptr, &syncStatus);
		} else {
			syncStatus = (m21_poll == GL_ALREADY_SIGNALED || m21_poll == GL_CONDITION_SATISFIED) ? GL_SIGNALED : GL_UNSIGNALED;
		}
#else
		glGetSynciv(state.canvas_instance_data_buffers[state.current_data_buffer_index].fence, GL_SYNC_STATUS, 1, nullptr, &syncStatus);
#endif // GODOT_PS5_M21_NONBLOCKING_CANVAS_FENCE"""

def patch(s: str) -> str:
    if TAG in s:
        if s.count(TAG) != 1 or "const GLenum m21_poll" not in s:
            raise ValueError("M21 canvas sync source partially patched")
        return s
    if s.count(ORIGINAL) != 1:
        raise ValueError("Unexpected pinned upstream Godot canvas fence query")
    return s.replace(ORIGINAL,PATCH,1)

def main() -> None:
    path=Path(sys.argv[1]) if len(sys.argv)>1 else Path("drivers/gles3/rasterizer_canvas_gles3.cpp")
    before=path.read_text()
    after=patch(before)
    if after!=before:
        path.write_text(after)
        print("GODOT_PS5_M21_CANVAS_FENCE_POLL_NONBLOCKING")
    else:
        print("GODOT_PS5_M21_CANVAS_FENCE_ALREADY_PATCHED")

if __name__=="__main__":
    main()
