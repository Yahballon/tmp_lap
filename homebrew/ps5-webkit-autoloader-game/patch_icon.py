#!/usr/bin/env python3
"""Make upstream v0.4.0 icon generator preserve our user-supplied PS5 game icon."""
from pathlib import Path
import argparse

def patch(root: Path) -> None:
    icon = root / "assets/icon-custom.png"
    generator = root / "tools/gen_icons.py"
    data = icon.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("Expected a real PNG for game icon")
    width, height = int.from_bytes(data[16:20], 'big'), int.from_bytes(data[20:24], 'big')
    if (width, height) != (512, 512):
        raise ValueError(f"Game icon must be 512x512; got {width}x{height}")
    text = generator.read_text()
    anchor = '    print("Generated icon assets from assets/icon.svg:")'
    if text.count(anchor) != 1:
        raise ValueError("Unrecognized upstream icon generator")
    insertion = (
        '    # Installed by PS5 Game music variant: override generated PNG ONLY.\n'
        '    # Other generated ICO/favicons remain upstream originals.\n'
        '    user_icon = os.path.join(ROOT, "assets", "icon-custom.png")\n'
        '    shutil.copyfile(user_icon, ICON0)\n'
        '    print("Using user-supplied 512x512 PS5 Game tile icon:", user_icon)\n\n'
    )
    generator.write_text(text.replace(anchor, insertion + anchor, 1))
    print(f"Custom PS5 Game icon registered ({len(data)} bytes), source is {icon}")

if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('upstream', type=Path)
    patch(p.parse_args().upstream)
