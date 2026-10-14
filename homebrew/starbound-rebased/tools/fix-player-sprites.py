#!/usr/bin/env python3
"""Gold872 player atlas repair for Starbound Rebased (build-time, PIL).

Gold872 addresses player frames at (1 + col*16, 9 + row*16).
Running IDs 2/3/4 (small), 27/28/29 (super) and 227/228/229 (fire)
must be views of ONE side-facing character; left is an SDL horizontal flip.
Mushroom transition IDs 46/45/25 must each show one 16x32 figure.
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SHEET = ROOT / 'res/sprites/characters/PlayerSpriteSheet.png'
ART = ROOT / 'hd-assets/characters/PlayerSpriteSheet.png'
SMALL_ROWS = (0,)
# Actual PlayerSystem: Super=25.. (row 1), Fire=225.. (row 9, NOT row 10).
BIG_ROWS = (1, 9)
WALK_COLS = (2, 3, 4)
# Player IDs 246/247 are projectiles, 248/249 are special effects.
SPECIAL = {(9, 21), (9, 22), (9, 23), (9, 24)}
NEAREST = Image.Resampling.NEAREST


def source_pose(art, group):
    """Select the sideways-facing original HD artist frame (column 2)."""
    for candidate in (group, 0):
        x, y = 2 + 2 * 32, 18 + candidate * 32
        pose = art.crop((x, y, x + 32, y + 32))
        if pose.getchannel('A').getbbox():
            return pose
    raise ValueError('Side-view astronaut missing from user-supplied HD atlas')


def animate_boots(pose, phase):
    """Only boots move. Helmet, face and torso cannot spin."""
    if phase == 0:
        return pose.copy()
    w, h = pose.size
    foot_y = h - (3 if h == 16 else 7)
    out = pose.copy()
    boots = pose.crop((0, foot_y, w, h))
    out.paste((0, 0, 0, 0), (0, foot_y, w, h))
    out.paste(boots, (phase, foot_y))
    return out


def place(atlas, col, row, sprite):
    x, y = 1 + col * 16, 9 + row * 16
    width, height = sprite.size
    assert width == 16 and height in (16, 32)
    assert x + width <= atlas.width and y + height <= atlas.height
    # Clear the whole source rectangle, including lower half of big form.
    atlas.paste((0, 0, 0, 0), (x, y, x + width, y + height))
    atlas.paste(sprite, (x, y))


def main():
    art = Image.open(ART).convert('RGBA')
    atlas = Image.open(SHEET).convert('RGBA')
    assert art.size == (806, 532), art.size
    assert atlas.size == (403, 266), atlas.size
    projectile_before = {}
    for row, col in SPECIAL:
        box = (1 + col*16, 9 + row*16, 17 + col*16, 25 + row*16)
        projectile_before[(row, col)] = atlas.crop(box).tobytes()

    # All in-game small player poses keep one orientation; otherwise changing
    # between standing/walking frames makes the character spin.
    for row in SMALL_ROWS:
        side = source_pose(art, 0).resize((16, 16), NEAREST)
        for col in range(25):
            pose = (animate_boots(side, (-1, 0, 1)[col - 2])
                    if col in WALK_COLS else side)
            place(atlas, col, row, pose)

    # Super uses row 1, Fire uses row 9. Row 9 must be a 16x32 TOP ROW;
    # the previous renderer asset generator painted row 9 as small 16x16
    # and row 10 as a second character, resulting in TWO stacked models.
    for row in BIG_ROWS:
        group = 0 if row == 1 else 3
        side = source_pose(art, group).resize((16, 32), NEAREST)
        for col in range(25):
            if (row, col) in SPECIAL:
                continue
            pose = (animate_boots(side, (-1, 0, 1)[col - 2])
                    if col in WALK_COLS else side)
            place(atlas, col, row, pose)

    # Fire-flower transformation IDs 350..353 also read a 16x32 rectangle.
    flower = source_pose(art, 3).resize((16, 32), NEAREST)
    for col in range(4):
        place(atlas, col, 14, flower)

    for (row, col), before in projectile_before.items():
        x, y = 1 + col*16, 9 + row*16
        assert atlas.crop((x, y, x + 16, y + 16)).tobytes() == before, 'Projectile overwritten'
    for row in SMALL_ROWS + BIG_ROWS:
        y = 9 + row*16
        head_height = 9 if row == 0 else 20
        tops = [atlas.crop((1+col*16, y, 17+col*16, y+head_height)).tobytes()
                for col in WALK_COLS]
        assert tops[0] == tops[1] == tops[2], ('walking astronaut rotates', row)
    for row in BIG_ROWS:
        y = 9 + row*16
        stand = atlas.crop((1, y, 17, y+32))
        assert stand.getchannel('A').crop((0, 0, 16, 16)).getbbox(), row
        assert stand.getchannel('A').crop((0, 16, 16, 32)).getbbox(), row
        for col in ((0, 20, 21) if row == 1 else (0, 18)):
            x = 1+col*16
            assert atlas.crop((x, y, x+16, y+32)).tobytes() == stand.tobytes(), (row, col)
    for col in range(4):
        x, y = 1 + col * 16, 9 + 14 * 16
        assert atlas.crop((x, y, x + 16, y + 32)).tobytes() == flower.tobytes(), col
    atlas.save(SHEET, optimize=True)
    print('PASS: sideways walking animation; single 16x32 mushroom/fire player; projectiles preserved')


if __name__ == '__main__':
    main()
