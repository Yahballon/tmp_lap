#!/usr/bin/env python3
"""Prevent unsupported PS5 filesystem syscalls from being compiled into storage path."""
import pathlib
import subprocess
import tempfile

src=pathlib.Path("homebrew/ps5-torrent/native")
with tempfile.TemporaryDirectory(prefix="torrent-native-check-") as d:
    tmp=pathlib.Path(d)
    (tmp/"ca_bundle.inc").write_text('static const char kCaBundle[]="";')
    output=subprocess.check_output([
        "g++","-E","-P","-std=c++17","-I",str(src),"-I",str(tmp),
        str(src/"torrent_engine.cpp")
    ],text=True)
    for begin,end in (("bool pathInfo(","bool safeDirTree("),
                      ("int openSafeRegular(","class OutputLayout{")):
        a=output.rfind(begin)
        b=output.find(end,a+len(begin))
        assert a>=0 and b>a,f"missing storage section: {begin}"
        block=output[a:b]
        for forbidden in ("lstat(", "readlink(", "O_NOFOLLOW"):
            assert forbidden not in block,f"unsafe native PS5 syscall or flag: {forbidden} in {begin}"
        assert "stat(" in block or "open(" in block
    print("PASS: native PS5 storage compiles without lstat/readlink/O_NOFOLLOW")
