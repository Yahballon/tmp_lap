#!/usr/bin/env python3
"""Actual C++ BitTorrent v1 multi-file integration test.

Validates BEP 3 stream mapping across file boundaries, nested folders,
zero-length files, rehash/recovery, and malicious path rejection.
"""
import hashlib
import pathlib
import shutil
import subprocess
import tempfile

import test_local_swarm as swarm

ROOT=swarm.ROOT
SRC=swarm.SOURCE

def run_harness(exe, torrent, success=True, timeout=40):
    p=subprocess.run([str(exe),str(torrent)],capture_output=True,text=True,timeout=timeout)
    if success and p.returncode!=0:
        raise AssertionError(f"expected transfer to succeed:\n{p.stdout}\n{p.stderr}")
    if not success and p.returncode==0:
        raise AssertionError(f"unexpectedly accepted malicious metadata:\n{p.stdout}")
    return p

def main():
    shutil.rmtree(ROOT,ignore_errors=True)
    ROOT.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="torrent-multifile-host-") as raw:
        tmp=pathlib.Path(raw)
        (tmp/"ca_bundle.inc").write_text('static const char kCaBundle[]="";')
        exe=tmp/"torrent_test"
        subprocess.run([
            "g++","-std=c++17","-O2","-pthread","-DTORRENT_HOST_TEST",
            "-I",str(SRC/"native"),"-I",str(tmp),
            str(SRC/"native/torrent_engine.cpp"),
            str(SRC/"tests/host_harness.cpp"),
            "-o",str(exe),"-lssl","-lcrypto"
        ],check=True)
        # Piece 0 crosses from nested ISO into checksum; piece 1 crosses
        # checksum into README. A zero-byte file also must be materialized.
        parts=[
            ([b"images",b"fedora.iso"],12021),
            ([b"checksums",b"SHA256SUMS"],swarm.PIECE+497),
            ([b"notes",b"README.txt"],len(swarm.CONTENT)-12021-swarm.PIECE-497),
            ([b"notes",b"EMPTY"],0)
        ]
        assert sum(size for _,size in parts)==len(swarm.CONTENT)
        info={
            b"name":b"Fedora-test-bundle",
            b"files":[{b"path":path,b"length":size} for path,size in parts],
            b"piece length":swarm.PIECE,
            b"pieces":swarm.HASHES,
        }
        swarm.INFO_HASH=hashlib.sha1(swarm.benc(info)).digest()
        a=swarm.make_server(swarm.Peer,pieces={0,2})
        b=swarm.make_server(swarm.Peer,pieces={1})
        tracker=swarm.make_server(swarm.Tracker,ports=[a.server_address[1],b.server_address[1]])
        try:
            fixture=tmp/"fedora-multifile.torrent"
            def torrent_info(blob):
                return {b"announce":f"http://127.0.0.1:{tracker.server_address[1]}/announce".encode(),b"info":blob}
            fixture.write_bytes(swarm.benc(torrent_info(info)))
            base=ROOT/"downloads/Fedora-test-bundle"
            print("Multi-file 1: real verified transfer across two file boundaries",flush=True)
            run_harness(exe,fixture,timeout=50)
            cursor=0
            for components,n in parts:
                path=base.joinpath(*(x.decode() for x in components))
                assert path.is_file(),f"output missing: {path}"
                assert path.read_bytes()==swarm.CONTENT[cursor:cursor+n],f"wrong bytes in {path}"
                cursor+=n
            print("Multi-file 2: corrupt a secondary file and re-download its piece",flush=True)
            secondary=base/"checksums/SHA256SUMS"
            with secondary.open("r+b") as f:
                f.seek(11)
                f.write(b"\x00\xff")
            run_harness(exe,fixture,timeout=50)
            cursor=0
            for pathParts,n in parts:
                path=base.joinpath(*(x.decode() for x in pathParts))
                assert path.read_bytes()==swarm.CONTENT[cursor:cursor+n]
                cursor+=n
            print("Multi-file 3: already verified data, no tracker required",flush=True)
            tracker.shutdown()
            tracker.server_close()
            run_harness(exe,fixture,timeout=18)

            print("Multi-file 4: traversal, duplicate, absolute and symlink guards",flush=True)
            variants=[
                [{b"path":[b"..",b"escape"],b"length":len(swarm.CONTENT)}],
                [{b"path":[b"../../escaped.iso"],b"length":len(swarm.CONTENT)}],
                [{b"path":[b"ok",b"/absolute"],b"length":len(swarm.CONTENT)}],
                [{b"path":[b"same"],b"length":1},{b"path":[b"same"],b"length":len(swarm.CONTENT)-1}],
                [{b"path":[b"same"],b"length":1},{b"path":[b"same",b"child"],b"length":len(swarm.CONTENT)-1}],
            ]
            for i,files in enumerate(variants):
                malicious=dict(info)
                malicious[b"name"]=f"unsafe-{i}".encode()
                malicious[b"files"]=files
                p=tmp/f"unsafe-{i}.torrent"
                p.write_bytes(swarm.benc(torrent_info(malicious)))
                run_harness(exe,p,success=False,timeout=8)
                assert not (ROOT/"downloads"/f"unsafe-{i}").exists(),f"unsafe tree created for {i}"
            link=ROOT/"downloads"/"symlink-test"
            link.symlink_to(tmp, target_is_directory=True)
            symlink_info=dict(info)
            symlink_info[b"name"]=b"symlink-test"
            p=tmp/"symlink.torrent"
            p.write_bytes(swarm.benc(torrent_info(symlink_info)))
            run_harness(exe,p,success=False,timeout=8)
            assert not (tmp/"images").exists(),"symlink escape created external directory"
            print("PASS: v1 multi-file mapping, SHA-1 cross-boundary, recheck, malicious-path refusal",flush=True)
        finally:
            a.shutdown()
            b.shutdown()
            a.server_close()
            b.server_close()

if __name__=="__main__":
    main()
