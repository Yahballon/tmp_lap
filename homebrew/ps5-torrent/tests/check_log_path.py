#!/usr/bin/env python3
"""Regression check: application diagnostics must stay in data storage."""
from pathlib import Path
source=Path("homebrew/ps5-torrent/native/torrent_engine.cpp").read_text()
begin=source.index("std::string ensureLogPath(){")
end=source.index("void trace(",begin)
impl=source[begin:end]
assert 'mkdirs("/data/ps5-torrent/logs")' in impl
assert 'return "/data/ps5-torrent/logs/torrent.log";' in impl
assert "/data/homebrew/" not in impl, "Logs must not be written into the installed app"
assert "/data/ps5-torrent/torrent.log" not in impl, "No unrequested fallback directory"
config=Path("homebrew/ps5-torrent/app.json").read_text()
assert '"logs": "/data/ps5-torrent/logs"' in config
print("PASS: Torrent Flow always uses /data/ps5-torrent/logs/torrent.log")
