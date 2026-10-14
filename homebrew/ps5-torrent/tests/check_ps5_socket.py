#!/usr/bin/env python3
"""Prevent the PS5 TCP regression from being reintroduced."""
from pathlib import Path
s=Path("homebrew/ps5-torrent/native/torrent_engine.cpp").read_text()
a=s.index("int dial(")
b=s.index("bool sendAll(",a)
body=s[a:b]
code_only="\n".join(line.split("//",1)[0] for line in body.splitlines())
assert "fcntl(" not in code_only, "PS5 sandbox rejects fcntl() on these TCP sockets"
assert "SOCK_NONBLOCK" in body, "Retain cancellable nonblocking code for host-only testing"
assert "#ifdef TORRENT_HOST_TEST" in body, "Fast connection mode must be gated off on PS5"
assert "PS5 reliable blocking TCP socket" in body, "Native path must use proven blocking sockets"
assert "bool asyncConnect=false;" in body, "Native PS5 sockets must default to blocking"
assert body.index("#ifdef TORRENT_HOST_TEST") < body.index("int enable=1;"), "FIONBIO attempt must be host-only"
assert "SO_SNDTIMEO" in body and "SO_RCVTIMEO" in body, "Missing socket timeouts"
assert body.index("SO_SNDTIMEO")<body.index("connect(fd,"), "Timeouts must precede blocking connect"
assert "TCP CONNECTED" in body and "TCP connect FAILED" in body, "Missing result diagnostics"
print("PASS: PS5 uses the reliable blocking TCP path; host-only async remains tested")
