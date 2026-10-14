#!/usr/bin/env python3
"""Real C++ integration check: dead or empty trackers must not be hammered.

Uses localhost only. A 503 or valid zero-peer response must receive one
announce, then respect cooldown while the downloader tries to find peers.
"""
import hashlib
import pathlib
import shutil
import socketserver
import subprocess
import tempfile
import threading

import test_local_swarm as swarm

class ControlledTracker(socketserver.BaseRequestHandler):
    def handle(self):
        self.request.settimeout(5)
        try:
            request=b""
            while b"\r\n\r\n" not in request and len(request)<8192:
                chunk=self.request.recv(1024)
                if not chunk:return
                request+=chunk
            with self.server.lock:
                self.server.requests.append(request)
            if self.server.unavailable:
                body=b"busy"
                status=b"503 Service Unavailable"
            else:
                body=swarm.benc({b"interval":1800,b"min interval":120,b"peers":b""})
                status=b"200 OK"
            self.request.sendall(b"HTTP/1.1 "+status+b"\r\nConnection: close\r\nContent-Length: "+
                                 str(len(body)).encode()+b"\r\n\r\n"+body)
        except (ConnectionError,OSError):
            pass

def main():
    shutil.rmtree(swarm.ROOT,ignore_errors=True)
    swarm.ROOT.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="torrent-tracker-cooldown-") as raw:
        tmp=pathlib.Path(raw)
        (tmp/"ca_bundle.inc").write_text('static const char kCaBundle[]="";')
        exe=tmp/"torrent_test"
        subprocess.run(["g++","-std=c++17","-O2","-pthread","-DTORRENT_HOST_TEST",
                        "-I",str(swarm.SOURCE/"native"),"-I",str(tmp),
                        str(swarm.SOURCE/"native/torrent_engine.cpp"),
                        str(swarm.SOURCE/"tests/host_harness.cpp"),
                        "-o",str(exe),"-lssl","-lcrypto"],check=True)
        for unavailable in (False,True):
            server=swarm.make_server(ControlledTracker)
            server.lock=threading.Lock()
            server.requests=[]
            server.unavailable=unavailable
            try:
                info={b"name":b"cooldown-test.iso",b"length":len(swarm.CONTENT),
                      b"piece length":swarm.PIECE,b"pieces":swarm.HASHES}
                metadata={b"info":info,
                          b"announce":f"http://127.0.0.1:{server.server_address[1]}/announce".encode()}
                torrent=tmp/("503.torrent" if unavailable else "zero-peers.torrent")
                torrent.write_bytes(swarm.benc(metadata))
                p=subprocess.run([str(exe),str(torrent)],capture_output=True,text=True,timeout=35)
                assert p.returncode==4,(p.returncode,p.stdout,p.stderr)
                assert len(server.requests)==1,(len(server.requests),p.stdout,p.stderr)
                assert b"event=started" in server.requests[0],"first announce must mark started"
                print("PASS:",("503" if unavailable else "zero peer"),"only one announce, backoff respected",flush=True)
            finally:
                server.shutdown();server.server_close()

if __name__=="__main__":
    main()
