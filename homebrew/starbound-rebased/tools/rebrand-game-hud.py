#!/usr/bin/env python3
"""Change game HUD string without touching Gold872 movement/physics."""
from pathlib import Path
import sys
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path('build/gold872-engine')
p=root/'src/systems/ScoreSystem.cpp'
s=p.read_text()
old='marioText->addComponent<TextComponent>("MARIO", 16);'
assert s.count(old)==1,"HUD label moved upstream; fail safely"
p.write_text(s.replace(old,'marioText->addComponent<TextComponent>("YAHBALLON", 16);'))
print("Replaced MARIO HUD label with YAHBALLON")
