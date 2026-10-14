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

def fixtures(work,count=2):
    files=[]
    servers=[]
    for index in range(count):
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
            print("Test queue: nine simultaneously active independent torrents",flush=True)
            shutil.rmtree(ROOT,ignore_errors=True)
            ROOT.mkdir(parents=True)
            nine_work=work/"nine-fixtures"
            nine_work.mkdir()
            nine_files,nine_servers=fixtures(nine_work,9)
            try:
                subprocess.run([str(exe),"--nine",*[str(x) for x in nine_files]],
                               check=True,timeout=55)
                for i in range(9):
                    assert (ROOT/"downloads"/f"queuetest-{i}.iso").read_bytes()==swarm.CONTENT
            finally:
                for server in nine_servers:
                    server.shutdown();server.server_close()
            print("Test queue: restart persisted history",flush=True)
            subprocess.run([str(exe),"--history"],check=True,timeout=10)
            print("Test metadata: original .torrent removed after enqueuing",flush=True)
            shutil.rmtree(ROOT,ignore_errors=True)
            ROOT.mkdir(parents=True)
            temporary_meta=work/"source-will-disappear.torrent"
            shutil.copy2(files[0],temporary_meta)
            subprocess.run([str(exe),"--metadata-seed",str(temporary_meta)],check=True,timeout=8)
            cached=list((ROOT/"cache"/"metainfo").glob("*.torrent"))
            assert len(cached)==1,"metadata was not durably cached at enqueue"
            temporary_meta.unlink()
            subprocess.run([str(exe),"--metadata-recover"],check=True,timeout=25)
            assert (ROOT/"downloads"/"queuetest-0.iso").read_bytes()==swarm.CONTENT
            assert not cached[0].exists(),"complete job retained redundant .torrent cache"
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
            print("Test auto cancel: watched metadata must not restart cancelled task",flush=True)
            shutil.rmtree(ROOT,ignore_errors=True)
            (ROOT/"torrents").mkdir(parents=True)
            watched=ROOT/"torrents"/files[0].name
            info_hash=hashlib.sha1(swarm.benc({
                b"length":len(swarm.CONTENT),
                b"name":b"queuetest-0.iso",
                b"piece length":swarm.PIECE,
                b"pieces":swarm.HASHES
            })).digest()
            stall=swarm.make_server(swarm.StallPeer,info_hash=info_hash)
            stall_tracker=swarm.make_server(swarm.Tracker,ports=[stall.server_address[1]])
            try:
                meta={
                    b"announce":f"http://127.0.0.1:{stall_tracker.server_address[1]}/announce".encode(),
                    b"info":{b"length":len(swarm.CONTENT),b"name":b"queuetest-0.iso",
                             b"piece length":swarm.PIECE,b"pieces":swarm.HASHES}
                }
                watched.write_bytes(swarm.benc(meta))
                subprocess.run([str(exe),"--cancel-auto"],check=True,timeout=18)
                assert watched.exists(),"cancelled .torrent metadata must be preserved"
            finally:
                stall_tracker.shutdown()
                stall_tracker.server_close()
                stall.shutdown()
                stall.server_close()
            print("Test crash recovery: SIGSYS-like abrupt exit must not relaunch queue",flush=True)
            shutil.rmtree(ROOT,ignore_errors=True)
            ROOT.mkdir(parents=True)
            subprocess.run([str(exe),"--crash-create",str(files[0])],check=True,timeout=8)
            assert (ROOT/"cache/session.active").exists(),"crash marker missing"
            subprocess.run([str(exe),"--check-recovery"],check=True,timeout=8)
            assert (ROOT/"cache/recovery.hold").exists(),"persistent recovery hold missing"
            subprocess.run([str(exe),"--resume-recovery"],check=True,timeout=8)
            assert not (ROOT/"cache/recovery.hold").exists(),"recovery hold not cleared"
            subprocess.run([str(exe),"--check-clean"],check=True,timeout=8)
            print("PASS: nine concurrent transfers, durable metainfo, auto-import and crash recovery",flush=True)
        finally:
            for server in servers:
                server.shutdown()
                server.server_close()

if __name__=="__main__":
    main()
