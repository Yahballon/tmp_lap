#!/usr/bin/env python3
"""Test actual C++ QueueManager: concurrent downloads, watched inbox and cleanup.

All network traffic is localhost, with two independent BitTorrent infohashes.
No Internet torrent download is made.
"""
import hashlib
import pathlib
import shutil
import subprocess
import tempfile
import test_local_swarm as swarm

ROOT = swarm.ROOT
SRC = swarm.SOURCE

def compile_harness(work):
    (work/"ca_bundle.inc").write_text('static const char kCaBundle[]="";')
    exe=work/"queue_test"
    subprocess.run([
        "g++","-std=c++17","-O2","-pthread","-DTORRENT_HOST_TEST",
        "-I",str(SRC/"native"),"-I",str(work),
        str(SRC/"native/torrent_engine.cpp"),
        str(SRC/"native/download_manager.cpp"),
        str(SRC/"tests/queue_harness.cpp"),
        "-o",str(exe),"-lssl","-lcrypto"
    ],check=True)
    return exe

def fixtures(work):
    files=[]
    servers=[]
    for index in range(2):
        name=f"queuetest-{index}.iso"
        info={
            b"length":len(swarm.CONTENT),
            b"name":name.encode(),
            b"piece length":swarm.PIECE,
            b"pieces":swarm.HASHES
        }
        digest=hashlib.sha1(swarm.benc(info)).digest()
        peer=swarm.make_server(swarm.Peer,pieces={0,1,2},info_hash=digest)
        tracker=swarm.make_server(swarm.Tracker,ports=[peer.server_address[1]])
        metadata={b"announce":f"http://127.0.0.1:{tracker.server_address[1]}/announce".encode(), b"info":info}
        path=work/(name+".torrent")
        path.write_bytes(swarm.benc(metadata))
        files.append(path)
        servers.extend((peer,tracker))
    return files,servers

def main():
    shutil.rmtree(ROOT,ignore_errors=True)
    ROOT.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="torrent-queue-test-") as raw:
        work=pathlib.Path(raw)
        exe=compile_harness(work)
        files,servers=fixtures(work)
        try:
            print("Test queue: two independent simultaneous torrents",flush=True)
            subprocess.run([str(exe),"--multi",str(files[0]),str(files[1])],check=True,timeout=45)
            for i in range(2):
                output=ROOT/"downloads"/f"queuetest-{i}.iso"
                assert output.read_bytes()==swarm.CONTENT, f"bad output {i}"
            print("Test queue: restart persisted history",flush=True)
            subprocess.run([str(exe),"--history"],check=True,timeout=10)
            # New clean queue for independent auto-inbox validation.
            shutil.rmtree(ROOT,ignore_errors=True)
            (ROOT/"torrents").mkdir(parents=True)
            watched=ROOT/"torrents"/files[0].name
            shutil.copy2(files[0],watched)
            print("Test auto: watch inbox, download, delete .torrent only on success",flush=True)
            subprocess.run([str(exe),"--auto"],check=True,timeout=45)
            assert not watched.exists(),"completed .torrent metadata was not removed"
            assert (ROOT/"downloads"/"queuetest-0.iso").read_bytes()==swarm.CONTENT
            subprocess.run([str(exe),"--history"],check=True,timeout=10)
            print("PASS: queue concurrency, persistence, auto watch and safe cleanup",flush=True)
        finally:
            for server in servers:
                server.shutdown()
                server.server_close()

if __name__=="__main__":
    main()
