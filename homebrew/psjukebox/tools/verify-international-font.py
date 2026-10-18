#!/usr/bin/env python3
"""Verify baked Unicode glyph table is real; a basic ASCII face is NOT enough."""
from pathlib import Path
import struct,sys
blob=Path(sys.argv[1]).read_bytes()
assert len(blob)>=40,"Missing .huifont header"
magic,version,w,h,pixels,spread,ascent,descent,gap,glyphs,kerns=struct.unpack_from('<IIHHfffffII',blob,0)
assert (magic,version)==(0x46505a50,1)
assert 256<=w<=4096 and 256<=h<=4096
assert len(blob)==40+24*glyphs+12*kerns+w*h
cps={struct.unpack_from('<I',blob,40+i*24)[0] for i in range(glyphs)}
samples={
 "German umlauts": "äöüÄÖÜß",
 "French accents": "éèêëàçù",
 "Spanish Portuguese": "ñáíóúãõ",
 "Polish Czech": "łżźćčřšě",
 "Turkish Romanian": "ğışİȘț",
 "Greek": "Ελληνικά",
 "Cyrillic Ukrainian": "Музыка Україна",
 "Nordic": "øæåÅ",
}
for label,sample in samples.items():
 missing=[f'U+{ord(c):04X}' for c in sample if not c.isspace() and ord(c) not in cps]
 assert not missing,f'{label}: missing {missing}'
print(f"PASS Unicode SDF atlas {w}x{h}, {len(cps)} glyphs; European/Greek/Cyrillic samples all rendered")
