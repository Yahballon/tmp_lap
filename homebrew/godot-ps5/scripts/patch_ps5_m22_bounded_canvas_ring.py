#!/usr/bin/env python3
"""M22: bound Godot GLES3 PS5 Canvas2D instance-data buffer-name ring.

Physical M21 TAR #41 shows 6 glBufferData calls per frame for Pong even
after warm-up. Godot upstream's _allocate_instance_data_buffer() inserts a
new multi-megabyte GL buffer set indefinitely when recent fences do not
signal. PS5/Mesa driver may lack timely fence retirement and exhaust its
268MiB shared GPU arena near the ~90 frame mark.

On the *experimental native PS5 GLES3* path only, bound this ever-growing
set to nine entries and use OpenGL buffer orphaning (glBufferData(...,nullptr))
on the next existing set. The old backing storage remains eligible for
in-flight draws in a conforming GL implementation. No GPU wait, manual
fence completion, unsafe write to a live mapped storage, or cap elsewhere.
"""
from pathlib import Path
import sys

TAG="GODOT_PS5_M22_BOUNDED_CANVAS_RING"
ANCHOR="void RasterizerCanvasGLES3::_allocate_instance_data_buffer() {\n"
CODE=r"""#if defined(PS5_ENABLED) && defined(GODOT_PS5_NATIVE_GLES3)
    // M22 controlled GPU-memory churn experiment. Keep a bounded number
    // of GL buffer object names; orphan current stores before writing.
    // Unlike reusing in-place with GL_MAP_UNSYNCHRONIZED_BIT, orphaning
    // preserves prior backing allocations for outstanding GPU commands.
    constexpr int m22_max_canvas_sets=9;
    if (state.canvas_instance_data_buffers.size() >= m22_max_canvas_sets) {
        state.current_data_buffer_index =
                (state.current_data_buffer_index + 1) % state.canvas_instance_data_buffers.size();
        DataBuffer &db=state.canvas_instance_data_buffers[state.current_data_buffer_index];
        if(db.fence != GLsync()) {
            glDeleteSync(db.fence); // Deletes handle only, not queued GPU work.
            db.fence=GLsync();
        }
        for(int i=0;i<db.instance_buffers.size();++i) {
            glBindBuffer(GL_ARRAY_BUFFER, db.instance_buffers[i]);
            glBufferData(GL_ARRAY_BUFFER,data.max_instance_buffer_size,nullptr,GL_STREAM_DRAW);
        }
        glBindBuffer(GL_UNIFORM_BUFFER,db.light_ubo);
        glBufferData(GL_UNIFORM_BUFFER,sizeof(LightUniform)*data.max_lights_per_render,
                nullptr,GL_STREAM_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER,db.state_ubo);
        glBufferData(GL_UNIFORM_BUFFER,sizeof(StateBuffer),nullptr,GL_STREAM_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER,0);
        glBindBuffer(GL_UNIFORM_BUFFER,0);
        db.last_frame_used=RSG::rasterizer->get_frame_number();
        static unsigned m22_reuses=0;
        ++m22_reuses;
        if(m22_reuses==1 || m22_reuses%60==0) {
            std::printf("GODOT_PS5_M22_CANVAS_RING cap=%d live_sets=%d orphan_reuses=%u\n",
                m22_max_canvas_sets,state.canvas_instance_data_buffers.size(),m22_reuses);
        }
        return;
    }
#endif // GODOT_PS5_M22_BOUNDED_CANVAS_RING
"""
def patch(s):
    if TAG in s:
        if s.count(TAG)!=1 or "m22_max_canvas_sets=9" not in s:
            raise ValueError("M22 patch partial or corrupted")
        return s
    if s.count(ANCHOR)!=1:
        raise ValueError("Missing pinned Godot 4.7.2 canvas instance ring anchor")
    if s.count("#include \"rasterizer_canvas_gles3.h\"")!=1:
        raise ValueError("Missing pinned Godot GLES3 canvas header")
    s=s.replace("#include \"rasterizer_canvas_gles3.h\"",
                '#include "rasterizer_canvas_gles3.h"\n#include <cstdio>',1)
    return s.replace(ANCHOR,ANCHOR+CODE,1)

def main():
    path=Path(sys.argv[1]) if len(sys.argv)>1 else Path("drivers/gles3/rasterizer_canvas_gles3.cpp")
    old=path.read_text()
    new=patch(old)
    if old!=new:
        path.write_text(new)
        print("GODOT_PS5_M22_CANVAS_RING_ORPHAN_PATCHED")
    else:
        print("GODOT_PS5_M22_CANVAS_RING_ALREADY_PATCHED")

if __name__=="__main__":
    main()
