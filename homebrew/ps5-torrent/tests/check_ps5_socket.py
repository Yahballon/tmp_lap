#!/usr/bin/env python3
"""Prevent the PS5 TCP regression from being reintroduced."""
from pathlib import Path
s=Path("homebrew/ps5-torrent/native/torrent_engine.cpp").read_text()
a=s.index("int dial(")
b=s.index("bool sendAll(",a)
body=s[a:b]
assert "fcntl(" not in body, "PS5 sandbox rejects fcntl() on these TCP sockets"
assert "O_NONBLOCK" in body, "Retain an explanation of the PS5 regression"
assert "SO_SNDTIMEO" in body and "SO_RCVTIMEO" in body, "Missing socket timeouts"
assert body.index("SO_SNDTIMEO")<body.index("connect(fd,"), "Timeouts must precede blocking connect"
assert "TCP CONNECTED" in body and "TCP connect FAILED" in body, "Missing result diagnostics"
print("PASS: PS5 blocking TCP socket path regression check")
