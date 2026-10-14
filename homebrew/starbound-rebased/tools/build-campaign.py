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
     ('steps reward patrol', 'gaps steps reward', 'reward patrol steps', 'steps gaps patrol')),
    ('Crystal Quarry', ('Fault Line', 'Crystal Steps', 'Cache Run', 'Quarry Relay'),
     ('gaps steps gaps', 'steps reward steps', 'reward gaps patrol', 'gaps steps patrol')),
    ('Spore Gardens', ('Green Frontier', 'Crawler Beds', 'Canopy Cache', 'Garden Relay'),
     ('patrol reward steps', 'steps patrol patrol', 'reward steps gaps', 'patrol gaps steps')),
    ('Cargo Ring', ('Dock Approach', 'Lift Yard', 'Upper Cargo', 'Freight Relay'),
     ('steps gaps patrol', 'lift steps lift', 'reward lift gaps', 'lift patrol steps')),
    ('Dark Observatory', ('Night Watch', 'Armored Gallery', 'Signal Cache', 'Lens Relay'),
     ('patrol steps gaps', 'armor gaps armor', 'reward armor steps', 'armor lift gaps')),
    ('Ion Foundry', ('Cooling Trench', 'Machine Floor', 'Maintenance Route', 'Forge Relay'),
     ('gaps armor steps', 'armor lift patrol', 'reward gaps lift', 'lift armor gaps')),
    ('Storm Array', ('Broken Links', 'High Voltage', 'Hidden Frequency', 'Storm Relay'),
     ('gaps lift armor', 'steps armor gaps', 'reward lift armor', 'armor gaps lift')),
    ('Relay Core', ('Core Approach', 'Security Grid', 'Reserve Cells', 'Signal Restored'),
     ('armor steps lift', 'lift armor gaps', 'reward armor gaps', 'gaps armor lift')),
)

def build(destination):
    manifest = {'title': 'Relayfall', 'version': 1, 'levels': []}
    for world, (sector, titles, plans) in enumerate(SECTORS, 1):
        for stage, (title, plan) in enumerate(zip(titles, plans), 1):
            width = 192 + (world - 1) * 4
            layers = {name: [[-1] * width for _ in range(15)] for name in LAYERS}
            fg, items, enemies = (layers[n] for n in ('Foreground', 'Collectibles', 'Enemies'))
            for x in range(width):
                fg[13][x] = 384
                fg[14][x] = 385
            def block(x, y, tile=197):
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
            # Six fixed bays: introduction, development, checkpoint, mastery.
            sequence = plan.split() * 2
            for bay, kind in enumerate(sequence):
                x = 26 + bay * 24
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
                    # Optional lift over a safe floor: waiting is never required.
                    block(x + 7, 10, 609)
                    moving.append(f'({x + 7}, 10) BACK_AND_FORTH UP (7, 11) FALSE')
                    for xx in range(x + 13, x + 18):
                        block(xx, 8)
                    coins(x + 13, 6, 5)
                else:
                    raise ValueError(kind)
                encounters.append({'kind': kind, 'x': x, 'end': x + 20})
            # A cleared service pad separates the two halves. Checkpoint is only
            # activated on this pad, with room for a full-size player.
            checkpoint = 98
            for yy in range(5, 13):
                for xx in range(95, 101):
                    fg[yy][xx] = items[yy][xx] = enemies[yy][xx] = -1
            crate(103)
            gate = width - 15
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
            if world == 1 and stage == 1:
                texts += [(8, 7, 'BUMP ENERGY CRATES'), (25, 6, 'HOLD JUMP TO RISE')]
            # CAMERA_MAX is the absolute right boundary, NOT scroll distance.
            props = f'PLAYER_START = (2, 12)\nCAMERA_START = (0, 0)\nCAMERA_MAX = {width - 1}\nLEVEL_TYPE = OVERWORLD\nNEXT_LEVEL = ({nxt[0]}, {nxt[1]})\nBACKGROUND_COLOR = BLUE\nCHECKPOINT = ({checkpoint}, 12)\n'
            for key, entries in (('FLOATING_TEXT', [f'({x}, {y}) ({text})' for x,y,text in texts]),
                                 ('MOVING_PLATFORM', moving), ('WARP_PIPE', []),
                                 ('PLATFORM_LEVEL', []), ('FIRE_BAR', []), ('VINE', [])):
                props += key + ' = \\\n' + ''.join(entry + '\n' for entry in entries) + '\n'
            path = destination / name
            path.mkdir(parents=True, exist_ok=True)
            for layer, rows in layers.items():
                (path / f'{name}_{layer}.csv').write_text('\n'.join(','.join(map(str, row)) for row in rows) + '\n', encoding='utf-8')
            (path / f'{name}.levelproperties').write_text(props, encoding='utf-8')
            manifest['levels'].append({'id': name, 'sector': sector, 'title': title, 'width': width,
                                       'next': list(nxt), 'checkpoint': [checkpoint, 12],
                                       'encounters': encounters})
    (destination / 'campaign.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    return manifest

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=ROOT / 'campaign')
    args = parser.parse_args()
    build(args.output)
    print('Authored Relayfall: 8 sectors, 32 stages, 192 encounter bays')
