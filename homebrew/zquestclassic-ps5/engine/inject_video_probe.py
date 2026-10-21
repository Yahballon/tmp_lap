#!/usr/bin/env python3
"""Inject a staging-only 8-bit framebuffer test into native PS5 UI.

This transforms a COPY in the engine CI job. It does not modify the public
TESTBUILDS app or claim upstream game frame output.
"""
import argparse
from pathlib import Path

argp = argparse.ArgumentParser()
argp.add_argument("staged_main", type=Path)
args = argp.parse_args()
text = args.staged_main.read_text()

def replace_once(old: str, new: str) -> None:
    global text
    assert text.count(old) == 1, f"Expected one occurrence of {old!r}"
    text = text.replace(old, new, 1)

replace_once(
    '#include "quest_index.hpp"',
    '#include "quest_index.hpp"\n#include "engine/ps5_video_probe.hpp"')

replace_once(
    '  QuestIndex scan;\n',
    '''  zcps5::engine::Ps5VideoProbe video_probe;
  const bool video_probe_ready = video_probe.init(renderer.batch());
  logging(video_probe_ready ? "PS5_INDEXED_VIDEO_BRIDGE_READY" :
                            "PS5_INDEXED_VIDEO_BRIDGE_FAILED");
  unsigned video_frame_counter = 0;
  QuestIndex scan;
''')

replace_once(
    '    scan.step(16);\n',
    '''    scan.step(16);
    if (video_probe_ready && ((video_frame_counter % 4u) == 0u))
      video_probe.draw_pattern(video_frame_counter / 4u);
    ++video_frame_counter;
''')

target = "    render_screen(ui, font, atlas, scan, reader, selected, show_info, show_status, exiting);\n"
replace_once(
    target,
    target + '''    if (video_probe_ready) {
      ui.image(video_probe.texture(), {1230.f, 346.f, 570.f, 335.f},
               {0.f, 0.f, 1.f, 1.f}, kWhite);
      txt(ui, font, atlas, "8-BIT PALETTE VIDEO TEST (NOT GAMEPLAY)",
          1230, 718, 17, kGold);
    }
''')

replace_once(
    '        pad.close();\n',
    '        video_probe.release();\n        pad.close();\n')

assert "PS5_INDEXED_VIDEO_BRIDGE_READY" in text
args.staged_main.write_text(text)
print("Injected EGL/OpenGL paletted frame test; no original gameplay substituted")
