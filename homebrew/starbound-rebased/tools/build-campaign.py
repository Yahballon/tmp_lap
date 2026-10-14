#!/usr/bin/env python3
"""Author the deterministic Relayfall campaign; no upstream map data is used."""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LAYERS = ('Above_Foreground', 'Background', 'Collectibles', 'Enemies', 'Foreground', 'Underground')
# Encounter order is authored, rather than chosen randomly. Each sector teaches
# a skill, develops it, offers a reward route, then combines it at the relay.
SECTORS = (
    ('Landing Reach', ('Arrival', 'Supply Trail', 'Survey Ridge', 'First Relay'),
     ('steps reward patrol steps valley reward', 'gaps steps reward patrol valley steps', 'reward patrol steps tunnel reward gaps', 'steps gaps patrol valley steps reward')),
    ('Crystal Quarry', ('Fault Line', 'Crystal Steps', 'Cache Run', 'Quarry Relay'),
     ('gaps steps tunnel reward valley gaps', 'steps reward valley tunnel steps patrol', 'reward gaps patrol tunnel valley reward', 'gaps steps patrol valley tunnel gaps')),
    ('Spore Gardens', ('Green Frontier', 'Crawler Beds', 'Canopy Cache', 'Garden Relay'),
     ('patrol reward steps tunnel valley patrol', 'steps patrol valley reward tunnel patrol', 'reward steps gaps patrol valley reward', 'patrol gaps steps tunnel reward valley')),
    ('Cargo Ring', ('Dock Approach', 'Lift Yard', 'Upper Cargo', 'Freight Relay'),
     ('steps gaps drone reward sky steps', 'lift steps sky drone reward lift', 'reward lift gaps sky drone reward', 'lift drone steps sky gaps patrol')),
    ('Tidal Observatory', ('Dive Tutorial', 'Coral Channels', 'Deep Cache', 'Sunken Lens'),
     ('reef swim reef cache swim reef', 'swim reef jelly swim cache reef', 'cache reef swim jelly reef cache', 'reef jelly swim cache jelly reef')),
    ('Ion Foundry', ('Cooling Trench', 'Machine Floor', 'Maintenance Route', 'Forge Relay'),
     ('gaps armor tunnel steps drone reward', 'armor lift patrol tunnel gaps drone', 'reward gaps lift tunnel armor sky', 'steps armor lift drone reward tunnel')),
    ('Storm Array', ('Broken Links', 'High Voltage', 'Hidden Frequency', 'Storm Relay'),
     ('gaps lift armor sky drone valley', 'steps armor gaps drone sky tunnel', 'reward lift drone sky armor reward', 'armor gaps lift valley drone sky')),
    ('Relay Core', ('Core Approach', 'Security Grid', 'Reserve Cells', 'Signal Restored'),
     ('armor steps lift drone tunnel gaps', 'lift armor gaps tunnel sky drone', 'reward armor gaps sky lift reward', 'gaps armor lift tunnel drone steps')),
)

def build(destination):
    manifest = {'title': 'Relayfall - Moving Frontiers', 'version': 3, 'levels': []}
    for world, (sector, titles, plans) in enumerate(SECTORS, 1):
        for stage, (title, plan) in enumerate(zip(titles, plans), 1):
            water = world == 5
            boss = (world, stage) in ((6, 4), (8, 4))
            ferry = world in (4,7,8)
            width = (264 if boss else 224) + (world - 1) * 4 + (stage - 1) * 6 + (64 if ferry else 0)
            biome = 'underwater' if water else ('reactor' if world in (6, 8) else ('crystal' if world == 2 else 'surface'))
            ground = {'surface':384, 'crystal':400, 'underwater':416, 'reactor':912}[biome]
            panel = {'surface':197, 'crystal':213, 'underwater':229, 'reactor':725}[biome]
            layers = {name: [[-1] * width for _ in range(15)] for name in LAYERS}
            fg, items, enemies = (layers[n] for n in ('Foreground', 'Collectibles', 'Enemies'))
            for x in range(width):
                fg[13][x] = ground
                fg[14][x] = ground + 1
                if water:
                    fg[0][x] = panel
            def block(x, y, tile=None):
                tile = panel if tile is None else tile
                fg[y][x] = tile
            def crate(x, item=608, y=9):
                block(x, y, 192)
                items[y][x] = item
            def coins(x, y, count=4):
                for xx in range(x, x + count):
                    block(xx, y, 144)
            crate(10)  # Growth is available before any hostile encounter.
            coins(16, 10, 3)
            encounters = []
            moving = []
            movers = []
            def mover(x,y,axis,minimum,maximum,required=False):
                # 809 is a real three-tile platform; 609 was a static block.
                block(x,y,809)
                direction='UP' if axis=='vertical' else 'RIGHT'
                moving.append(f'({x}, {y}) BACK_AND_FORTH {direction} ({minimum}, {maximum+2 if axis=="horizontal" else maximum}) FALSE')
                movers.append({'x':x,'y':y,'axis':axis,'min':minimum,'max':maximum,
                               'width':3,'required':required})
            # Six fixed bays: introduction, development, checkpoint, mastery.
            sequence = plan.split()
            assert len(sequence) == 6
            if not water:
                sequence[3]='lift_gate'
            for bay, kind in enumerate(sequence):
                x = (26, 52, 78, 112, 140, 168)[bay] + (stage - 1 if bay != 2 else 0)
                if kind == 'gaps':
                    span = 2 if world <= 2 else (3 if world <= 5 else 4)
                    for xx in range(x + 7, x + 7 + span):
                        fg[13][xx] = fg[14][xx] = -1
                    coins(x + 6, 9, span + 2)
                elif kind == 'steps':
                    # One-tile rises with broad treads; no powerup required.
                    for offset, height in ((3, 1), (6, 2), (9, 3), (12, 2), (15, 1)):
                        for xx in range(x + offset, x + offset + 3):
                            for yy in range(13 - height, 13):
                                block(xx, yy)
                    coins(x + 9, 8, 3)
                elif kind in ('patrol', 'armor'):
                    ids = [70] if world < 3 else [70, 38]
                    if kind == 'armor':
                        ids = [87, 38]
                    for offset, eid in zip((8, 16), ids):
                        enemies[11 if eid == 38 else 12][x + offset] = eid
                    # A visible raised escape perch provides a combat bypass.
                    for xx in range(x + 3, x + 6):
                        block(xx, 11)
                    coins(x + 3, 9, 3)
                    if kind == 'armor':
                        crate(x + 1, 96)  # Optional temporary invulnerability.
                elif kind == 'reward':
                    for xx in range(x + 2, x + 5):
                        block(xx, 11)
                    for xx in range(x + 7, x + 13):
                        block(xx, 9)
                    coins(x + 7, 7, 6)
                    # Hidden reserve life above the reward shelf, never mandatory.
                    items[6][x + 10] = 52
                elif kind == 'lift':
                    mover(x+7,10,'vertical',7,10)
                    for xx in range(x + 13, x + 18):
                        block(xx, 8)
                    coins(x + 13, 6, 5)
                elif kind == 'lift_gate':
                    # Seven-to-nine-tile rises exceed a ground jump. Board
                    # the lift from its bottom phase, ride up, jump to the ledge.
                    for xx in range(x+1,x+24):
                        for yy in range(2,13):
                            fg[yy][xx]=items[yy][xx]=enemies[yy][xx]=-1
                    ledge_top=4+(world+stage)%3
                    mover(x+7+stage%2,12,'vertical',ledge_top+1,12,True)
                    for xx in range(x+14,x+18):
                        for yy in range(ledge_top,15): block(xx,yy)
                    for xx in range(x+18,x+22): block(xx,8)
                    coins(x+14,ledge_top-2,4)
                elif kind == 'valley':
                    for offset in (4, 11):
                        for xx in range(x + offset, x + offset + 3):
                            fg[13][xx] = fg[14][xx] = -1
                        coins(x + offset, 9, 3)
                    for xx in range(x + 7, x + 11):
                        block(xx, 12)
                elif kind == 'tunnel':
                    for xx in range(x + 3, x + 17):
                        block(xx, 8)
                    coins(x + 5, 11, 5)
                    if world >= 3:
                        enemies[12][x + 14] = 87 if world >= 6 else 70
                elif kind == 'sky':
                    for xx in range(x + 5, x + 14):
                        fg[13][xx] = fg[14][xx] = -1
                        block(xx, 11)
                    for xx in range(x + 2, x + 5):
                        block(xx, 12)
                    coins(x + 6, 9, 6)
                elif kind == 'drone':
                    enemies[8][x + 14] = 90
                    for xx in range(x + 4, x + 9):
                        block(xx, 11)
                    coins(x + 4, 9, 4)
                elif kind in ('reef', 'swim', 'jelly', 'cache'):
                    if kind == 'reef':
                        # Alternating coral pillars guide a readable vertical swim.
                        for xx in range(x + 5, x + 8):
                            for yy in range(9,13): block(xx, yy)
                        for xx in range(x + 13, x + 16):
                            for yy in range(1,5): block(xx, yy)
                        coins(x + 7, 6, 5)
                    elif kind == 'swim':
                        enemies[7][x + 14] = 81
                        coins(x + 3, 9, 6)
                    elif kind == 'jelly':
                        enemies[5][x + 14] = 48
                        coins(x + 3, 10, 5)
                        for xx in range(x + 4, x + 8): block(xx, 3)
                    else:
                        for xx in range(x + 5, x + 13): block(xx, 8)
                        coins(x + 6, 5, 6)
                        items[4][x + 9] = 52
                else:
                    raise ValueError(kind)
                encounters.append({'kind': kind, 'x': x, 'end': x + 20})
            # Each ordinary bay gets an approach and exit patrol. Patrol species
            # escalate by sector, but mandatory boarding/landing zones stay clear.
            for bay, encounter in enumerate(encounters):
                x,kind=encounter['x'],encounter['kind']
                if kind=='lift_gate': continue
                for offset in (1,20):
                    xx=x+offset
                    if water:
                        yy,eid=(8,81)
                        if fg[yy][xx] != -1: continue
                    else:
                        eid=38 if world>=3 and bay%2 else (87 if world>=6 else 70)
                        yy=11 if eid==38 else 12
                        if fg[13][xx] not in (384,400,416,912) or any(fg[y][xx]!=-1 for y in range(yy,13)):
                            continue
                    if enemies[yy][xx]==-1: enemies[yy][xx]=eid
            ferry_span=None
            if ferry:
                left=width-(102 if boss else 78)
                right=left+28
                for xx in range(left-3,right+3):
                    for yy in range(2,13):
                        fg[yy][xx]=items[yy][xx]=enemies[yy][xx]=-1
                for xx in range(left,right):
                    fg[13][xx]=fg[14][xx]=-1
                mover(left-2,11,'horizontal',left-2,right-1,True)
                coins(left,7,5)
                coins(right-5,7,5)
                ferry_span=[left,right]
            # A pre-checkpoint perch patrol and short tail encounters prevent
            # quiet half-screens without crowding the actual checkpoint/exit.
            for xx in [88]+list(range(200,width-58,18)):
                if ferry_span and ferry_span[0]-4<=xx<ferry_span[1]+4: continue
                if water:
                    if fg[8][xx]==-1: enemies[8][xx]=81
                else:
                    floor=next((yy for yy in range(3,14) if fg[yy][xx] not in (-1,144,809)),None)
                    if floor is not None and fg[floor-1][xx] in (-1,144): enemies[floor-1][xx]=70
            # A cleared service pad separates the two halves. Checkpoint is only
            # activated on this pad, with room for a full-size player.
            checkpoint = 98
            for yy in range(5, 13):
                for xx in range(95, 101):
                    fg[yy][xx] = items[yy][xx] = enemies[yy][xx] = -1
            for yy in range(15):
                for xx in range(92,105): enemies[yy][xx] = -1
            crate(103)
            gate = width - 15
            if boss:
                # Reactor cut-off terminal collapses the arena beneath the
                # sentinel. The high maintenance route works without fireballs.
                bridge_start, bridge_end = width - 48, width - 18
                for xx in range(bridge_start, bridge_end + 1):
                    fg[13][xx], fg[14][xx] = 392, -1
                for xx in range(bridge_start-6, bridge_start-3): block(xx, 11)
                for xx in range(bridge_start-3, bridge_start): block(xx, 9)
                for xx in range(bridge_start, bridge_end + 1):
                    block(xx, 8)
                coins(bridge_start+3, 6, 8)
                enemies[11][width-32] = 61
                block(bridge_end, 11, 339)
                block(width-11, 12, 240)
                crate(bridge_start-10)
            else:
                for y in range(5, 13):
                    block(gate, y, 149)
                block(gate - 1, 5, 152)
            # FlagSystem finishes only after auto-walking into a right wall.
            for y in range(9, 13):
                block(width - 5, y)
            name = f'World{world}-{stage}'
            nxt = (world, stage + 1) if stage < 4 else ((world + 1, 1) if world < 8 else (0, 0))
            texts = [(2, 4, sector), (2, 5, title),
                     (92, 6, 'SERVICE CHECKPOINT'), (gate - 7, 4, 'RELAY ONLINE')]
            if water:
                texts[3] = (gate-8,4,'SURFACE AIRLOCK')
                texts.append((12,6,'TAP JUMP TO SWIM'))
            else:
                texts.append((encounters[3]['x']+1,6,'BOARD LIFT THEN JUMP'))
            if ferry:
                texts.append((ferry_span[0]-8,6,'JUMP ON THE MOVING FERRY'))
            if boss:
                texts[3] = (width-57,4,'REACTOR CUTOFF AHEAD')
                texts.append((width-54,5,'TAKE THE UPPER ROUTE'))
            if world == 1 and stage == 1:
                texts += [(8, 7, 'BUMP ENERGY CRATES'), (25, 6, 'HOLD JUMP TO RISE')]
            # CAMERA_MAX is the absolute right boundary, NOT scroll distance.
            level_type = 'UNDERWATER' if water else ('CASTLE' if boss else 'OVERWORLD')
            props = f'PLAYER_START = (2, 12)\nCAMERA_START = (0, 0)\nCAMERA_MAX = {width - 1}\nLEVEL_TYPE = {level_type}\nNEXT_LEVEL = ({nxt[0]}, {nxt[1]})\nBACKGROUND_COLOR = BLUE\nCHECKPOINT = ({checkpoint}, 12)\n'
            for key, entries in (('FLOATING_TEXT', [f'({x}, {y}) ({text})' for x,y,text in texts]),
                                 ('MOVING_PLATFORM', moving), ('WARP_PIPE', []),
                                 ('PLATFORM_LEVEL', []), ('FIRE_BAR', []), ('VINE', [])):
                props += key + ' = \\\n' + ''.join(entry + '\n' for entry in entries) + '\n'
            path = destination / name
            path.mkdir(parents=True, exist_ok=True)
            for layer, rows in layers.items():
                (path / f'{name}_{layer}.csv').write_text('\n'.join(','.join(map(str, row)) for row in rows) + '\n', encoding='utf-8',newline='\n')
            (path / f'{name}.levelproperties').write_text(props, encoding='utf-8',newline='\n')
            manifest['levels'].append({'id': name, 'sector': sector, 'title': title, 'width': width,
                                       'next': list(nxt), 'checkpoint': [checkpoint, 12],
                                       'biome':biome, 'level_type':level_type, 'boss':boss,
                                       'movers':movers,'ferry_span':ferry_span,
                                       'enemy_count':sum(n!=-1 for row in enemies for n in row),
                                       'encounters': encounters})
    (destination / 'campaign.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8',newline='\n')
    return manifest

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=ROOT / 'campaign')
    args = parser.parse_args()
    build(args.output)
    print('Authored Moving Frontiers: dense encounters, 28 required lifts and 12 mandatory ferries')
