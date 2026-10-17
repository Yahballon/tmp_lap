#!/usr/bin/env python3
"""Prepare and validate PS5 game selection/launch background DDS assets.

Usage:
    python3 prepare_background.py <source-image> <upstream>/assets --prepare
    python3 prepare_background.py <upstream>/assets/pic0.dds --verify

The encoder is richgel999/bc7enc_rdo (built separately in CI).
The PS5 presentation format is 3840x2160 BC7 DX10 DDS with one mip level.
"""
import argparse
from pathlib import Path
import struct


W, H = 3840, 2160
DDS_LEN = 148 + (W // 4) * (H // 4) * 16


def prepare(source: Path, output: Path) -> None:
    from PIL import Image, ImageOps
    output.mkdir(parents=True, exist_ok=True)
    with Image.open(source) as orig:
        orig.load()
        if orig.width < 1024 or orig.height < 576:
            raise ValueError("Background must be at least 1024x576")
        rgb = orig.convert("RGB")
        fitted = ImageOps.fit(
            rgb, (W, H), method=Image.Resampling.LANCZOS,
            centering=(0.5, 0.5),
        )
        path = output / "background-4k.png"
        fitted.save(path, optimize=True)
        print(f"Prepared PS5 4K input: {source} -> {path} ({path.stat().st_size} bytes)")


def verify(source: Path) -> None:
    data = source.read_bytes()
    if len(data) != DDS_LEN:
        raise ValueError(f"Incorrect 4K BC7 DDS length: {len(data)} (expected {DDS_LEN})")
    if data[:4] != b"DDS " or struct.unpack_from("<I", data, 4)[0] != 124:
        raise ValueError("Missing DDS header")
    height, width = struct.unpack_from("<II", data, 12)
    if (width, height) != (W, H):
        raise ValueError(f"Wrong DDS dimensions {width}x{height}")
    # Legacy DDS pixel format FOURCC must be DX10 and DXGI format BC7_UNORM=98.
    if data[84:88] != b"DX10":
        raise ValueError("Expected DX10 extended pixel format")
    dxgi, dimension, misc, array_size, misc2 = struct.unpack_from("<IIIII", data, 128)
    if (dxgi, dimension, array_size) != (98, 3, 1):
        raise ValueError(f"Expected BC7_UNORM 2D array_size=1, got {dxgi}, {dimension}, {array_size}")
    mipmaps = struct.unpack_from("<I", data, 28)[0]
    if mipmaps not in (0, 1):
        raise ValueError("Background must be single-mip")
    print(f"Validated PS5 BC7 DDS: {source} ({width}x{height}, {len(data)} bytes)")


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("source", type=Path)
    p.add_argument("output", type=Path, nargs="?")
    mode = p.add_mutually_exclusive_group(required=True)
    mode.add_argument("--prepare", action="store_true")
    mode.add_argument("--verify", action="store_true")
    args = p.parse_args()
    if args.prepare:
        if args.output is None:
            p.error("--prepare requires an output directory")
        prepare(args.source, args.output)
    else:
        verify(args.source)


if __name__ == "__main__":
    main()
