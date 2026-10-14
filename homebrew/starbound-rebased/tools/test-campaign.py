#!/usr/bin/env python3
"""Regression tests proving the content validator rejects broken campaign data."""
import contextlib
import importlib.util
import io
import tempfile
from pathlib import Path
from build_campaign_import import build

spec = importlib.util.spec_from_file_location('checker', Path(__file__).with_name('check-campaign.py'))
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)

with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    build(root)
    props = root/'World1-1/World1-1.levelproperties'
    original = props.read_text()
    for old,new in [('NEXT_LEVEL = (1, 2)','NEXT_LEVEL = (8, 4)'),
                    ('CHECKPOINT = (98, 12)','CHECKPOINT = (30, 12)'),
                    ('CAMERA_MAX = 223','CAMERA_MAX = 199')]:
        props.write_text(original.replace(old,new))
        try:
            with contextlib.redirect_stdout(io.StringIO()):
                checker.validate(root, reproducible=False)
        except AssertionError:
            pass
        else:
            raise AssertionError(f'Validator accepted broken property {old}')
    props.write_text(original)
    fg = root/'World1-1/World1-1_Foreground.csv'
    original = fg.read_text()
    for old,new in [('152','-1'), ('384','999')]:
        fg.write_text(original.replace(old,new))
        try:
            checker.validate(root, reproducible=False)
        except AssertionError:
            pass
        else:
            raise AssertionError(f'Validator accepted broken tile {old}')
    fg.write_text(original)
    wall = [[-1]*60 for _ in range(15)]
    for x in range(60):
        wall[13][x] = 384
    for y in range(15):
        wall[y][25] = 197
    assert not checker.reachable(wall,1), 'Route test crossed an impassable wall'
    assert not checker.reachable(wall,2), 'Large route test crossed a wall'
print('PASS: rejected broken transitions, camera limits, checkpoints, IDs, exits and impassable routes')
