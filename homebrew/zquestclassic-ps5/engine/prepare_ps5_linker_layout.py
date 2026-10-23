#!/usr/bin/env python3
"""Reserve genuine PS5 process parameters in a staged native PIE linker map.

The official PS5-DevBridge converter inserts SIE process-parameter structures
between the ELF RELRO and .dynamic sections. Large zplayer static archives
leave <1KiB in that area despite the working small-app linker script's normal
page alignment. Add explicit virtual-address room *before .dynamic* so the
same converter can insert real process parameters. The normal public UI
build and upstream engine source are not modified.

This does not claim successful FSELF signing or console boot.
"""
import argparse
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument("staged_linker_script",type=Path)
args=p.parse_args()
path=args.staged_linker_script
data=path.read_text()
old="    . = ALIGN(CONSTANT(MAXPAGESIZE));\n    .dynamic : {"
new="""    . = ALIGN(CONSTANT(MAXPAGESIZE));
    /* PS5 module writer inserts <1KiB of real SIE process parameters
     * between RELRO/GOT and dynamic. Reserve 64KiB for the full zplayer's
     * larger RELRO and any linker-generated orphan read-only sections. */
    . += 0x10000;
    .dynamic : {"""
assert data.count(old)==1,"Pinned PS5 native PIE linker map changed; inspect before altering"
assert data.count("    .dynamic : {")==1
assert "PHDRS {" in data and "dynamic PT_DYNAMIC" in data
path.write_text(data.replace(old,new,1))
print("PS5_LINKER_PROCESS_PARAM_GAP=0x10000 (real converter-compatible layout)")
