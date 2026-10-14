#!/usr/bin/env python3
"""Validate bundled public BitTorrent v1 single-file fixtures before packaging."""
from __future__ import annotations
import hashlib
import json
import pathlib
import sys

# Current supported BitTorrent v1 single-file fixtures from official mirrors.
# Specific metadata SHA256 hashes are pinned after first CI retrieval and validation.
NAMES = (
    "ubuntu-26.04.1-desktop-amd64.iso.torrent",
    "ubuntu-24.04.5.1-desktop-amd64.iso.torrent",
    "lmde-7-cinnamon-64bit.iso.torrent",
    "kubuntu-26.04.1-desktop-amd64.iso.torrent",
    "xubuntu-26.04.1-desktop-amd64.iso.torrent",
    "lubuntu-26.04.1-desktop-amd64.iso.torrent",
)
EXPECTED_SHA256 = {}

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
        assert isinstance(info, dict) and b"files" not in info, f"unsupported multi-file torrent: {path}"
        name = info[b"name"].decode("utf-8", "strict")
        assert name == filename[:-8], f"unexpected ISO: {name} in {filename}"
        size = info[b"length"]
        assert 1_000_000 < size <= 16*1024**3, f"unsupported ISO length: {name}: {size}"
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
        print(f"TORRENT {filename} | ISO {size:,} bytes | pieces {len(hashes)//20} | piece size {piece_len:,} | sha256 {hashlib.sha256(raw).hexdigest()}")
        print(f"TRACKERS {filename} | {json.dumps(trackers)}")
        entries.append(dict(filename=filename, metadata_sha256=hashlib.sha256(raw).hexdigest(), iso_name=name, iso_size_bytes=size, piece_length=piece_len, trackers=trackers))
    manifest = root/"verified-fixtures.json"
    manifest.write_text(json.dumps(entries, indent=2)+"\n")
    print("All 6 torrent fixtures validated successfully.")

if __name__ == "__main__":
    validate(pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else pathlib.Path("app/assets/test"))
