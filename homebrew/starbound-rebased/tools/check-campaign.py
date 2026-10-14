#!/usr/bin/env python3
"""Validate actual CSV semantics and conservative, static small/large routes.

The reachability model uses the pinned jump/gravity constants. It excludes
enemies, moving platforms and destructible shortcuts; hardware playtests are
still required for timing, enemy AI and input feel.
"""
import argparse
import csv
import json
import re
import tempfile
from collections import deque
from pathlib import Path
from build_campaign_import import build, LAYERS

ROOT = Path(__file__).resolve().parents[1]
NON_SOLID = {-1, 144, 149, 152, 609, 240, 339}

def swim_reachable(fg, height):
    """Static 4-connected swim corridor with actual one/two-tile body clearance."""
    width = len(fg[0])
    nodes = {(x,y) for x in range(width) for y in range(1,14-height+1)
             if all(fg[yy][x] in NON_SOLID for yy in range(y,y+height))}
    start = (2,13-height)
    seen,queue = {start},deque([start])
    while queue:
        x,y=queue.popleft()
        if x >= width-12:
            return True
        for node in {(x+1,y),(x-1,y),(x,y+1),(x,y-1)} & nodes - seen:
            seen.add(node)
            queue.append(node)
    return False

def reachable(fg, height):
    width = len(fg[0])
    # Standing states are feet coordinates at each accessible tile top.
    nodes = {(x, y) for y in range(2, 14) for x in range(width)
             if fg[y][x] not in NON_SOLID and
             all(fg[yy][x] in NON_SOLID for yy in range(y-height, y))}
    start = (2, 13)
    seen, queue = {start}, deque([start])
    while queue:
        x, y = queue.popleft()
        if x >= width - 12:
            return True
        candidates = {(x + 1, y)} & nodes
        # Held-jump trajectory: native 32px tiles, gravity .575, held -.412,
        # initial velocity -7.3 and fall cap 7.5. Forward movement is bounded.
        for speed in (3.0, 5.0, 7.0):
            px, feet, vy = x*32.0, y*32.0, -7.3
            for frame in range(120):
                prev = feet
                vy = min(7.5, vy + .575 + (-.412 if vy < -1 else 0))
                px += speed
                feet += vy
                xx = int((px + 16) // 32)
                if xx >= width or feet >= 480:
                    break
                top = feet - height*32
                # Check body interior against terrain, including ceiling hits.
                collision = False
                for yy in range(max(0, int(top//32)), min(15, int((feet-.01)//32)+1)):
                    if fg[yy][xx] not in NON_SOLID:
                        collision = True
                        break
                if vy > 0:
                    landed = next(((xx, yy) for yy in range(2,14)
                                   if (xx,yy) in nodes and prev <= yy*32 <= feet), None)
                    if landed:
                        candidates.add(landed)
                        break
                if collision:
                    break
        for node in candidates - seen:
            seen.add(node)
            queue.append(node)
    return False

def validate(data, reproducible=True):
    manifest = json.loads((data/'campaign.json').read_text())
    levels = manifest['levels']
    assert len(levels) == 32
    assert len({entry['id'] for entry in levels}) == 32
    fingerprints = set()
    for index, entry in enumerate(levels):
        name, width = entry['id'], entry['width']
        layers = {}
        for layer in LAYERS:
            path = data/name/f'{name}_{layer}.csv'
            rows = [[int(n) for n in row] for row in csv.reader(path.open())]
            assert len(rows) == 15 and all(len(row) == width for row in rows), path
            layers[layer] = rows
        fg, items, enemies = (layers[n] for n in ('Foreground','Collectibles','Enemies'))
        assert set(n for row in fg for n in row) <= {-1,144,149,152,192,197,213,229,725,384,385,400,401,416,417,912,913,609,240,339,392}
        assert set(n for row in enemies for n in row) <= {-1,38,70,87,90,81,48,61}
        assert set(n for row in items for n in row) <= {-1,52,96,608}
        props = (data/name/f'{name}.levelproperties').read_text()
        expected = [index//4 + 1,index%4 + 2] if index%4 < 3 else [index//4+2,1]
        if index == 31:
            expected = [0,0]
        assert entry['next'] == expected
        assert f'NEXT_LEVEL = ({expected[0]}, {expected[1]})' in props
        assert f'CAMERA_MAX = {width-1}' in props
        assert 'CHECKPOINT = (98, 12)' in props
        assert fg[13][2] == fg[13][98] in (384,400,416,912)
        assert f"LEVEL_TYPE = {entry['level_type']}" in props
        assert all(fg[y][x] == -1 and enemies[y][x] == -1 and items[y][x] == -1
                   for y in range(5,13) for x in range(95,101))
        if entry['boss']:
            assert entry['level_type'] == 'CASTLE'
            assert sum(row.count(61) for row in enemies) == 1
            assert sum(row.count(240) for row in fg) == 1
            assert sum(row.count(339) for row in fg) == 1
            assert all(fg[13][x]==392 and fg[14][x]==-1 for x in range(width-48,width-17))
            assert all(fg[8][x] not in NON_SOLID for x in range(width-48,width-17))
        else:
            assert sum(row.count(152) for row in fg) == 1
            assert all(fg[y][width-15] == 149 for y in range(5,13))
        assert all(fg[y][width-5] not in NON_SOLID for y in range(9,13))
        for y,row in enumerate(enemies):
            for x,eid in enumerate(row):
                if eid != -1:
                    feet = y + (2 if eid in (38,61) else 1)
                    if eid not in (48,81,90):
                        assert fg[feet][x] not in NON_SOLID, (name,'unsupported enemy',x,y)
                    else:
                        assert fg[y][x] in NON_SOLID, (name,'airborne enemy in terrain',x,y)
                    assert x >= 24 and abs(x-98) >= 6
        for y,row in enumerate(items):
            for x,item in enumerate(row):
                if item != -1:
                    assert fg[y][x] == 192 or (item == 52 and fg[y][x] == -1)
        lifts = [(x,y) for y,row in enumerate(fg) for x,tile in enumerate(row) if tile == 609]
        for x,y in lifts:
            assert f'({x}, {y}) BACK_AND_FORTH UP (7, 11) FALSE' in props
        assert len(re.findall(r'BACK_AND_FORTH', props)) == len(lifts)
        for size in (1,2):
            route=swim_reachable if entry['level_type']=='UNDERWATER' else reachable
            assert route(fg,size), (name,'no static route',size)
        fingerprint = tuple(tuple(row) for row in fg)
        fingerprints.add(fingerprint)
    assert len(fingerprints) == 32, 'Duplicate level geometry'
    if reproducible:
        with tempfile.TemporaryDirectory() as tmp:
            generated = Path(tmp)
            build(generated)
            for source in generated.rglob('*'):
                if source.is_file():
                    assert source.read_bytes() == (data/source.relative_to(generated)).read_bytes(), source
    assert sum(entry['boss'] for entry in levels)==2
    assert sum(entry['level_type']=='UNDERWATER' for entry in levels)==4
    print('PASS: 32 unique maps, 4 swim stages, 2 complete boss assemblies, IDs/links/pads/lifts; 64 routes')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('data', nargs='?', type=Path, default=ROOT/'campaign')
    args = parser.parse_args()
    validate(args.data)
