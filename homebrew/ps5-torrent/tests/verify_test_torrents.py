#!/usr/bin/env python3
"""Validate bundled public BitTorrent v1 single- or multi-file fixtures before packaging."""
from __future__ import annotations
import hashlib
import json
import pathlib
import sys

# Current official Linux fixtures plus archived originals for legacy queue migration.
# SHA-256 hashes pinned from successfully verified official CI downloads (2026-10-14).
NAMES = (
    # Exact six historical .torrent files from the v0.2.0 release.
    "debian-13.7.0-amd64-netinst.iso.torrent",
    "ubuntu-24.04.5-live-server-amd64.iso.torrent",
    "ubuntu-26.04.1-live-server-amd64.iso.torrent",
    "linuxmint-22.3-cinnamon-64bit.iso.torrent",
    "linuxmint-22.3-xfce-64bit.iso.torrent",
    "linuxmint-22.3-mate-64bit.iso.torrent",
    "ubuntu-26.04.1-desktop-amd64.iso.torrent",
    "ubuntu-24.04.5.1-desktop-amd64.iso.torrent",
    "Fedora-Workstation-Live-x86_64-44.torrent",
    "kubuntu-26.04.1-desktop-amd64.iso.torrent",
    "xubuntu-26.04.1-desktop-amd64.iso.torrent",
    "lubuntu-26.04.1-desktop-amd64.iso.torrent",
)
EXPECTED_SHA256 = {
    "debian-13.7.0-amd64-netinst.iso.torrent": "690700ed8de8d0987fcf8c136507471b1588b996696ca34f9cf972ee09b849e0",
    "ubuntu-24.04.5-live-server-amd64.iso.torrent": "81d2234a304fbed595c78df9ec243ae7695263f591f3dabedcb808e5cd7615c8",
    "ubuntu-26.04.1-live-server-amd64.iso.torrent": "6487c84e88f05ac802d72251213ec0e619df1c806c0303c4fa6e8780251b8640",
    "linuxmint-22.3-cinnamon-64bit.iso.torrent": "df4ed6d10da55456f72aa9c2d0d726b5f6b0fe2fe5d1bb304c756411a913f6c3",
    "linuxmint-22.3-xfce-64bit.iso.torrent": "11a8eb56a95c1aa7f1cde27e168aaa7ebcb6f4687eb2b5c112bd12acd1eb0d67",
    "linuxmint-22.3-mate-64bit.iso.torrent": "9b692fcbff8d8ca53e2eb1603e678e1b07f5a47ab4d3344a1ff068042211d722",
    "ubuntu-26.04.1-desktop-amd64.iso.torrent": "a4b59d67dfeffa07211b433cee5a0b1e545329b899be2c6cc83725cb13aafad6",
    "ubuntu-24.04.5.1-desktop-amd64.iso.torrent": "6e60d9967257d4cedd52747e09fd2468daea27538690a62c48169aebe3cc33d5",
    "Fedora-Workstation-Live-x86_64-44.torrent": "14d96ec3a1d56cef3eb2b64a5c7505459bbb6c0e6d88442e88c8d05dfb02bc79",
    "kubuntu-26.04.1-desktop-amd64.iso.torrent": "ebf081bbb9f69acb3ed8c64c426f4fb1d822c126504eac75a8a47bb6231dbd61",
    "xubuntu-26.04.1-desktop-amd64.iso.torrent": "8d3bf91f3209e1c7fa8a01758d25632f0cb107f14464477e054ec438fa3a4126",
    "lubuntu-26.04.1-desktop-amd64.iso.torrent": "861cc6aa484cdcc369b3d1dadb2d4dc459d0f252dd1496dc2febc6a9c8694c12",
}

def decode(data: bytes, offset: int = 0, depth: int = 0):
    if depth > 24 or offset >= len(data):
        raise ValueError("invalid bencode nesting")
    ch = data[offset:offset+1]
    if ch == b"i":
        end = data.index(b"e", offset)
        return int(data[offset+1:end]), end+1
    if ch in (b"d", b"l"):
        isdict = ch == b"d"
        result = {} if isdict else []
        offset += 1
        while data[offset:offset+1] != b"e":
            key, offset = decode(data, offset, depth+1)
            if isdict:
                if not isinstance(key, bytes):
                    raise ValueError("bad dictionary key")
                value, offset = decode(data, offset, depth+1)
                result[key] = value
            else:
                result.append(key)
        return result, offset + 1
    colon = data.index(b":", offset)
    n = int(data[offset:colon])
    if n < 0 or n > 1_048_576 or colon+1+n > len(data):
        raise ValueError("invalid bencode string length")
    return data[colon+1:colon+1+n], colon+1+n

def validate(root: pathlib.Path):
    assert root.is_dir(), f"missing fixture folder: {root}"
    expected = set(NAMES)
    observed = {p.name for p in root.glob("*.torrent")}
    assert observed == expected, f"fixture mismatch missing={expected-observed}, unexpected={observed-expected}"
    entries = []
    for filename in NAMES:
        path = root/filename
        raw = path.read_bytes()
        assert 100 < len(raw) < 1_048_576, f"incorrect torrent file size: {path}"
        if filename in EXPECTED_SHA256:
            assert hashlib.sha256(raw).hexdigest() == EXPECTED_SHA256[filename], f"torrent SHA-256 checksum changed for {filename}"
        meta, consumed = decode(raw)
        assert consumed == len(raw) and isinstance(meta, dict), f"bad torrent metadata: {path}"
        info = meta[b"info"]
        assert isinstance(info, dict), f"missing torrent info: {path}"
        name = info[b"name"].decode("utf-8", "strict")
        assert name and name != ".." and "/" not in name and "\\" not in name
        files = info.get(b"files")
        if files is not None:
            assert b"length" not in info, f"length and files present: {path}"
            assert isinstance(files,list) and 0 < len(files) <= 2048, f"invalid file list: {path}"
            total=0
            paths=set()
            for entry in files:
                assert isinstance(entry,dict) and isinstance(entry.get(b"length"),int)
                assert entry[b"length"]>=0
                components=entry[b"path"]
                assert isinstance(components,list) and 0<len(components)<=16
                components=[p.decode("utf-8","strict") for p in components]
                assert all(c and c not in (".","..") and ".." not in c and
                           not any(q in c for q in "\\/:") for c in components)
                rel="/".join(components)
                assert rel not in paths, "duplicate file path"
                paths.add(rel)
                total+=entry[b"length"]
            for rel in paths:
                assert all(not p.startswith(rel+"/") for p in paths if p!=rel), "file/dir overlap"
            size=total
            assert filename.startswith("Fedora-"), "unexpected multi-file torrent"
        else:
            assert name==filename[:-8], f"unexpected ISO: {name} in {filename}"
            size=info[b"length"]
        assert 1_000_000 < size <= 16*1024**3, f"unsupported torrent length: {name}: {size}"
        piece_len = info[b"piece length"]
        hashes = info[b"pieces"]
        assert 16_384 <= piece_len <= 4*1024**2, f"unsupported piece length: {name}: {piece_len}"
        assert len(hashes) % 20 == 0 and len(hashes)//20 == (size+piece_len-1)//piece_len, f"bad piece hashes: {name}"
        tracker = meta.get(b"announce", b"").decode("utf-8", "replace")
        trackers = [tracker]
        for tier in meta.get(b"announce-list", []):
            for val in tier:
                s = val.decode("utf-8", "replace")
                if s not in trackers:
                    trackers.append(s)
        print(f"TORRENT {filename} | {len(files) if files is not None else 1} file(s) | {size:,} bytes | pieces {len(hashes)//20} | piece size {piece_len:,} | sha256 {hashlib.sha256(raw).hexdigest()}")
        print(f"TRACKERS {filename} | {json.dumps(trackers)}")
        entries.append(dict(filename=filename, metadata_sha256=hashlib.sha256(raw).hexdigest(), torrent_name=name, total_bytes=size, file_count=len(files) if files is not None else 1, piece_length=piece_len, trackers=trackers))
    manifest = root/"verified-fixtures.json"
    manifest.write_text(json.dumps(entries, indent=2)+"\n")
    print("All 12 torrent fixtures validated successfully.")

if __name__ == "__main__":
    validate(pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else pathlib.Path("app/assets/test"))
