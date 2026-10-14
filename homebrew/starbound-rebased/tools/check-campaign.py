#!/usr/bin/env python3
"""Validate CSV semantics, swim clearance and moving-platform phase routes.

The route model uses pinned jump/gravity constants and boarding/ride/exit edges.
It excludes enemy AI and frame-perfect multi-platform timing; compiled physics
regressions and PS5 hardware playtests provide complementary coverage.
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
NON_SOLID = {-1, 144, 149, 152, 609, 761, 809, 240, 339}

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

def reachable(fg, height, movers=()):
    width = len(fg[0])
    # Standing states are feet coordinates at each accessible tile top.
    nodes = {(x, y) for y in range(2, 14) for x in range(width)
             if fg[y][x] not in NON_SOLID and
             all(fg[yy][x] in NON_SOLID for yy in range(y-height, y))}
    start = (2, 13)
    # Boarding/ride/exit graph over real platform phases. Riding connects only
    # clear phases of the SAME platform, never a phantom static bridge.
    ride_edges={}
    for platform in movers:
        states=set()
        for phase in range(platform['min'],platform['max']+1):
            left=phase if platform['axis']=='horizontal' else platform['x']
            top=phase if platform['axis']=='vertical' else platform['y']
            for xx in range(left,left+platform['width']):
                if 0<=xx<width and top>=height and all(fg[yy][xx] in NON_SOLID for yy in range(top-height,top)):
                    states.add((xx,top))
        for state in states:
            ride_edges.setdefault(state,set()).update(states)
    landing_nodes=nodes|ride_edges.keys()
    seen, queue = {start}, deque([start])
    while queue:
        x, y = queue.popleft()
        if x >= width - 12:
            return True
        candidates = ({(x + 1, y)} & landing_nodes) | ride_edges.get((x,y),set())
        # Held-jump trajectory: native 32px tiles, gravity .575, held -.412,
        # initial velocity -7.3 and fall cap 7.5. Forward movement is bounded.
        for speed in (3.0, 5.0, 7.0, 10.0):
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
                                   if (xx,yy) in landing_nodes and prev <= yy*32 <= feet), None)
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
        assert set(n for row in fg for n in row) <= {-1,144,149,152,192,197,213,229,725,384,385,400,401,416,417,912,913,809,240,339,392}
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
        movers=entry['movers']
        lifts = {(x,y) for y,row in enumerate(fg) for x,tile in enumerate(row) if tile == 809}
        assert lifts=={(m['x'],m['y']) for m in movers}
        for m in movers:
            direction='UP' if m['axis']=='vertical' else 'RIGHT'
            maximum=m['max']+2 if m['axis']=='horizontal' else m['max']
            assert f"({m['x']}, {m['y']}) BACK_AND_FORTH {direction} ({m['min']}, {maximum}) FALSE" in props
            assert m['width']==3 and m['min']<m['max']
        assert len(re.findall(r'BACK_AND_FORTH', props)) == len(lifts)
        assert entry['enemy_count']==sum(n!=-1 for row in enemies for n in row)
        assert entry['enemy_count']>=10, (name,'enemy density too low',entry['enemy_count'])
        for size in (1,2):
            if entry['level_type']=='UNDERWATER':
                assert swim_reachable(fg,size), (name,'no swim route',size)
            else:
                assert reachable(fg,size,movers), (name,'no boarding/ride/exit route',size)
                assert not reachable(fg,size), (name,'required platforms can be bypassed',size)
                assert any(m['required'] for m in movers)
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
    print('PASS: 32 dense maps, 64 routes; dry stages require actual platform boarding/ride/exit')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('data', nargs='?', type=Path, default=ROOT/'campaign')
    args = parser.parse_args()
    validate(args.data)
