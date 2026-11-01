#!/usr/bin/env python3
"""Build an RX-readable copy of RandomX x86 static code templates.

Native PS5 executes .text but intentionally cannot read its bytes. RandomX's
JitCompilerX86 normally reads template bytes out of .text via memcpy.
Create a 1:1 assembly *data* copy, keeping the original executable .S file.
Only template input pointers in jit_compiler_x86.cpp are redirected; the
execution/function symbols elsewhere are not changed.
"""
import re
from pathlib import Path
import sys

src=Path(sys.argv[1]).resolve() / "src"
p=src / "jit_compiler_x86_static.S"
cpp=src / "jit_compiler_x86.cpp"
asm=p.read_text()
orig=cpp.read_text()
assert asm.count(".section .text")==1
assert asm.count("#define DECL(x) x")==1
names=re.findall(r"^\.global DECL\((randomx_[A-Za-z0-9_]+)\)$",asm,re.M)
assert len(names)>=17 and len(names)==len(set(names)), names
data=asm.replace(".section .text", '.section .rodata.rxjit,"a",@progbits')
data=data.replace("#define DECL(x) x","#define DECL(x) rxblob_##x")
# Only PS5 crossbuild uses this duplicate source. Local assembly symbols
# stay local to their respective object, so both can coexist.
new_source=src / "jit_compiler_x86_readable.S"
new_source.write_text(data)

# The jit compiler uses every static template as data to copy into its
# separately allocated JIT buffer. Do NOT change symbols called as functions
# outside this translation unit (e.g. for non-JIT RandomX users).
declarations='\nextern "C" {\n'+''.join(
    f'  void rxblob_{name}();\n' for name in names
)+'}\n'
needle='#include "jit_compiler_x86_static.hpp"'
assert orig.count(needle)==1
orig=orig.replace(needle,needle+declarations)
for name in names:
    src_pattern=f"ADDR({name})"
    if src_pattern in orig:
        orig=orig.replace(src_pattern,f"ADDR(rxblob_{name})")
assert "ADDR(randomx_program_prologue)" not in orig
assert "ADDR(rxblob_randomx_program_prologue)" in orig
cpp.write_text(orig)
print("Generated readable RandomX ASM template copy:",new_source)
print("Redirected JIT copy sources for",len(names),"symbols")
