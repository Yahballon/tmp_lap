#!/usr/bin/env python3
"""Patch pinned ZiZc3 LiveShell source for standalone ThemeChanger use.

GPL-3.0 upstream remains attributed. Changes:
- PS5 Homebrew ThemeChanger isn't a game: don't pause wallpaper for PPSA99891
- No separate LiveShell Media tile: never install LVSH00001 on startup
No patch to ShellUI injection, filesystem mount, or playback pipeline.
"""
from pathlib import Path
import sys
if len(sys.argv)!=2:raise SystemExit("usage: patch_liveshell.py <LiveShell source dir>")
p=Path(sys.argv[1])/"src/liveshell_daemon.cpp"
s=p.read_text()
a='strcmp(id, "LVSH00001") != 0;'
b='strcmp(id, "LVSH00001") != 0 && strcmp(id, "PPSA99891") != 0;'
if s.count(a)!=1:raise SystemExit("Upstream LiveShell game detection has changed: refusing unsafe build")
s=s.replace(a,b,1)
a='start_thread(after_start_thread);'
b='log("ThemeChanger integration: LiveShell Media tile disabled (native app PPSA99891 used instead)");'
if s.count(a)!=1:raise SystemExit("Upstream LiveShell tile launch has changed: refusing unsafe build")
s=s.replace(a,b,1)
p.write_text(s)
assert "strcmp(id, \"PPSA99891\") != 0" in s
assert 'start_thread(after_start_thread);' not in s
print("Verified: ThemeChanger is exempt from game-pause detection")
print("Verified: LiveShell Engine Media tile will NOT auto-install")
