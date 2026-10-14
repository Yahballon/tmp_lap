#!/usr/bin/env python3
"""End-to-end tests with two local BitTorrent peers and an HTTP tracker.

Tests the actual C++ torrent engine, not a Python reimplementation:
  * two peers advertise DISJOINT available pieces (selection must not force piece 0)
  * CHOKE -> UNCHOKE transition, bitfield parsing, fragmented TCP frames
  * SHA1 block verification, disk persistence, resume after corruption
  * rehash-only completion when existing data is intact (no tracker required)
No public torrents, external peers, or network accounts are involved.
"""
import hashlib
import pathlib
import shutil
import socket
import socketserver
import struct
import subprocess
import tempfile
import threading

SOURCE = pathlib.Path(__file__).resolve().parents[1]
ROOT = pathlib.Path("/tmp/torrent-flow-host-test")
PIECE = 32768
CONTENT = bytes((i * 19 + i // 17) % 251 for i in range(PIECE * 2 + 7891))
HASHES = b"".join(hashlib.sha1(CONTENT[i:i+PIECE]).digest() for i in range(0, len(CONTENT), PIECE))
NPIECES = len(HASHES) // 20
INFO_HASH = None

def benc(x):
    if isinstance(x, int):
        return b"i" + str(x).encode() + b"e"
    if isinstance(x, bytes):
        return str(len(x)).encode() + b":" + x
    if isinstance(x, list):
        return b"l" + b"".join(benc(v) for v in x) + b"e"
    if isinstance(x, dict):
        return b"d" + b"".join(benc(k) + benc(v) for k, v in sorted(x.items())) + b"e"
    raise TypeError(type(x))

def recv_exact(sock, n):
    out = bytearray()
    while len(out) < n:
        p = sock.recv(n-len(out))
        if not p:
            raise EOFError("Connection closed")
        out += p
    return bytes(out)

def frame(msg, payload=b""):
    data = bytes([msg]) + payload
    return struct.pack(">I", len(data)) + data

class Peer(socketserver.BaseRequestHandler):
    def handle(self):
        sock=self.request
        sock.settimeout(10)
        has=self.server.pieces
        try:
            handshake=recv_exact(sock,68)
            if handshake[:20]!=b"\x13BitTorrent protocol" or handshake[28:48]!=getattr(self.server, "info_hash", INFO_HASH):
                raise AssertionError("client handshake was invalid")
            # Return a normal peer handshake, then choke, bitfield and unchoke.
            sock.sendall(b"\x13BitTorrent protocol"+b"\0"*8+getattr(self.server, "info_hash", INFO_HASH)+b"-MOCK00-ABCDEFGHIJKL")
            mask = sum(0x80 >> i for i in has)
            messages=frame(0)+frame(5,bytes([mask]))+frame(1)
            # Deliberately fragment message boundaries to exercise recvAll.
            sock.sendall(messages[:3])
            sock.sendall(messages[3:13])
            sock.sendall(messages[13:])
            while True:
                length=struct.unpack(">I",recv_exact(sock,4))[0]
                if length==0:
                    continue
                payload=recv_exact(sock,length)
                if payload[0]!=6:
                    continue
                if len(payload)!=13:
                    raise AssertionError("malformed client REQUEST")
                index, begin, blocklength=struct.unpack(">III",payload[1:])
                if index not in has:
                    raise AssertionError("requested piece not advertised by peer")
                if blocklength<1 or blocklength>16384 or begin%16384:
                    raise AssertionError("invalid request offset/size")
                start=index*PIECE+begin
                blob=CONTENT[start:start+blocklength]
                if len(blob)!=blocklength:
                    raise AssertionError("out-of-bounds block request")
                response=frame(7,struct.pack(">II",index,begin)+blob)
                sock.sendall(response[:11])
                sock.sendall(response[11:])
        except (ConnectionError,OSError,EOFError,socket.timeout):
            return

class StallPeer(socketserver.BaseRequestHandler):
    """Completes handshake and then deliberately keeps socket idle."""
    def handle(self):
        sock=self.request
        sock.settimeout(8)
        try:
            handshake=recv_exact(sock,68)
            if handshake[28:48]!=INFO_HASH:
                raise AssertionError("bad test info hash")
            sock.sendall(b"\x13BitTorrent protocol"+b"\0"*8+INFO_HASH+b"-STALL0-ABCDEFGHIJKL")
            sock.sendall(frame(5,b"\xe0")+frame(1))
            # Consume requests, but never deliver PIECE messages.
            threading.Event().wait(4.5)
        except (ConnectionError,OSError,EOFError,socket.timeout):
            pass

class Tracker(socketserver.BaseRequestHandler):
    def handle(self):
        self.request.settimeout(8)
        try:
            raw=b""
            while b"\r\n\r\n" not in raw and len(raw)<16384:
                raw+=self.request.recv(2048)
            if b"GET /announce?" not in raw:
                raise AssertionError("client did not announce")
            compact=b"".join(socket.inet_aton("127.0.0.1")+struct.pack(">H",p) for p in self.server.ports)
            payload=benc({b"interval":1800,b"peers":compact})
            result=(b"HTTP/1.1 200 OK\r\nContent-Length: "+str(len(payload)).encode()+
                    b"\r\nConnection: close\r\n\r\n"+payload)
            self.request.sendall(result)
        except (ConnectionError,OSError,socket.timeout):
            return

class Server(socketserver.ThreadingTCPServer):
    allow_reuse_address=True
    daemon_threads=True

def make_server(handler, **params):
    server=Server(("127.0.0.1",0),handler)
    for k,v in params.items():
        setattr(server,k,v)
    thread=threading.Thread(target=server.serve_forever,daemon=True)
    thread.start()
    return server

def main():
    shutil.rmtree(ROOT,ignore_errors=True)
    ROOT.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="torrent-flow-host-") as temp:
        temp=pathlib.Path(temp)
        (temp/"ca_bundle.inc").write_text('static const char kCaBundle[] = "";')
        executable=temp/"torrent_host_test"
        compiler=["g++","-std=c++17","-O2","-pthread","-DTORRENT_HOST_TEST",
                  "-I",str(SOURCE/"native"),"-I",str(temp),
                  str(SOURCE/"native/torrent_engine.cpp"),
                  str(SOURCE/"tests/host_harness.cpp"),
                  "-o",str(executable),"-lssl","-lcrypto"]
        subprocess.run(compiler,check=True)
        a=make_server(Peer,pieces={0,2})
        b=make_server(Peer,pieces={1})
        tracker=make_server(Tracker,ports=[a.server_address[1],b.server_address[1]])
        try:
            info={b"length":len(CONTENT),b"name":b"fixture.iso",b"piece length":PIECE,b"pieces":HASHES}
            global INFO_HASH
            INFO_HASH=hashlib.sha1(benc(info)).digest()
            metadata={b"announce":f"http://127.0.0.1:{tracker.server_address[1]}/announce".encode(),
                      b"info":info}
            fixture=temp/"fixture.iso.torrent"
            fixture.write_bytes(benc(metadata))
            dest=ROOT/"downloads/fixture.iso"
            print("Test 1: parallel peers, disjoint availability, choke/unchoke, SHA1",flush=True)
            subprocess.run([str(executable),str(fixture)],check=True,timeout=55)
            assert dest.read_bytes()==CONTENT,"download differs from reference data"
            print("Test 2: corrupt stored piece and verify repair on restart",flush=True)
            with dest.open("r+b") as f:
                f.seek(PIECE+137)
                f.write(b"\xff")
            subprocess.run([str(executable),str(fixture)],check=True,timeout=55)
            assert dest.read_bytes()==CONTENT,"corrupted piece was not repaired"
            print("Test 3: all pieces already verified; skip tracker",flush=True)
            tracker.shutdown()
            tracker.server_close()
            subprocess.run([str(executable),str(fixture)],check=True,timeout=20)
            assert dest.read_bytes()==CONTENT
            print("Test 4: Cancel interrupts a peer waiting for data",flush=True)
            stall=make_server(StallPeer)
            stalled_tracker=make_server(Tracker,ports=[stall.server_address[1]])
            try:
                dest.write_bytes(b"\0"*len(CONTENT))
                new_fixture=temp/"cancel.iso.torrent"
                new_fixture.write_bytes(benc({
                    b"announce":f"http://127.0.0.1:{stalled_tracker.server_address[1]}/announce".encode(),
                    b"info":info
                }))
                subprocess.run([str(executable),str(new_fixture),"--cancel"],check=True,timeout=8)
            finally:
                stalled_tracker.shutdown()
                stalled_tracker.server_close()
                stall.shutdown()
                stall.server_close()
            print("PASS: transfer, corrupt-piece repair, resume and rapid cancel",flush=True)
        finally:
            a.shutdown()
            b.shutdown()
            a.server_close()
            b.server_close()

if __name__=="__main__":
    main()
