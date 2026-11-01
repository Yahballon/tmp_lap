#!/usr/bin/env python3
"""Instrument ONLY the pinned RandomX checkout during PS5 cross-compilation.

Never edit the upstream repo in Git or unconditionally alter its crypto logic.
The hooks add durable console breadcrumbs only; RandomX's logic is unchanged.
"""
from pathlib import Path
import sys

root = Path(sys.argv[1]).resolve() / "src"

def patch(file, transformations):
    p=root/file
    text=p.read_text()
    for old,new in transformations:
        count=text.count(old)
        if count != 1:
            raise RuntimeError(f"{file}: expected exactly one anchor, found {count}: {old[:100]!r}")
        text=text.replace(old,new)
    p.write_text(text)

patch("randomx.cpp", [
    ('#include "randomx.h"', '#include "randomx.h"\nextern "C" void rx_ps5_jit_stage(const char*, unsigned long long);'),
    ('randomx_vm *randomx_create_vm(randomx_flags flags, randomx_cache *cache, randomx_dataset *dataset) {\n',
     'randomx_vm *randomx_create_vm(randomx_flags flags, randomx_cache *cache, randomx_dataset *dataset) {\n'
     '\t\trx_ps5_jit_stage("rx_create_enter", (unsigned long long)flags);\n'),
    ('\t\ttry {\n\t\t\tswitch ((int)(flags & (RANDOMX_FLAG_FULL_MEM | RANDOMX_FLAG_JIT | RANDOMX_FLAG_HARD_AES | RANDOMX_FLAG_LARGE_PAGES))) {',
     '\t\ttry {\n\t\t\trx_ps5_jit_stage("rx_before_switch_new", 0);\n'
     '\t\t\tswitch ((int)(flags & (RANDOMX_FLAG_FULL_MEM | RANDOMX_FLAG_JIT | RANDOMX_FLAG_HARD_AES | RANDOMX_FLAG_LARGE_PAGES))) {'),
    ('\t\t\tif(cache != nullptr) {\n\t\t\t\tvm->setCache(cache);',
     '\t\t\trx_ps5_jit_stage("rx_after_constructor", vm != nullptr);\n'
     '\t\t\tif(cache != nullptr) {\n\t\t\t\trx_ps5_jit_stage("rx_before_set_cache", 0);\n\t\t\t\tvm->setCache(cache);'),
    ('\t\t\tvm->allocate();\n\t\t}\n\t\tcatch (std::exception &ex) {',
     '\t\t\trx_ps5_jit_stage("rx_before_scratchpad_alloc", 0);\n'
     '\t\t\tvm->allocate();\n'
     '\t\t\trx_ps5_jit_stage("rx_after_scratchpad_alloc", 1);\n'
     '\t\t}\n\t\tcatch (std::exception &ex) {\n'
     '\t\t\trx_ps5_jit_stage("rx_exception", 1);')
])

patch("jit_compiler_x86.cpp", [
    ('#include "jit_compiler_x86.hpp"',
     '#include "jit_compiler_x86.hpp"\nextern "C" void rx_ps5_jit_stage(const char*, unsigned long long);'),
    ('JitCompilerX86::JitCompilerX86() {\n\t\tcode = (uint8_t*)allocMemoryPages(CodeSize);',
     'JitCompilerX86::JitCompilerX86() {\n'
     '\t\trx_ps5_jit_stage("jit_ctor_enter", 0);\n'
     '\t\trx_ps5_jit_stage("jit_before_code_mmap", (unsigned long long)CodeSize);\n'
     '\t\tcode = (uint8_t*)allocMemoryPages(CodeSize);\n'
     '\t\trx_ps5_jit_stage("jit_after_code_mmap", code != nullptr);'),
    ('\t\tmemcpy(code, codePrologue, prologueSize);\n\t\tmemcpy(code + epilogueOffset, codeEpilogue, epilogueSize);',
     '\t\trx_ps5_jit_stage("jit_before_prologue_copy", (unsigned long long)prologueSize);\n'
     '\t\tmemcpy(code, codePrologue, prologueSize);\n'
     '\t\trx_ps5_jit_stage("jit_after_prologue_copy", 1);\n'
     '\t\trx_ps5_jit_stage("jit_before_epilogue_copy", (unsigned long long)epilogueSize);\n'
     '\t\tmemcpy(code + epilogueOffset, codeEpilogue, epilogueSize);\n'
     '\t\trx_ps5_jit_stage("jit_after_epilogue_copy", 1);')
])

patch("virtual_memory.c", [
    ('#include "virtual_memory.h"',
     '#include "virtual_memory.h"\nextern void rx_ps5_jit_stage(const char*, unsigned long long);'),
    ('void* allocMemoryPages(size_t bytes) {\n\tvoid* mem;',
     'void* allocMemoryPages(size_t bytes) {\n'
     '\trx_ps5_jit_stage("page_alloc_enter", (unsigned long long)bytes);\n'
     '\tvoid* mem;'),
    ('\tmem = mmap(NULL, bytes, PAGE_READWRITE | RESERVED_FLAGS | PEXTRA, MAP_ANONYMOUS | MAP_PRIVATE | MEXTRA, -1, 0);\n\tif (mem == MAP_FAILED)\n\t\tmem = NULL;',
     '\tmem = mmap(NULL, bytes, PAGE_READWRITE | RESERVED_FLAGS | PEXTRA, MAP_ANONYMOUS | MAP_PRIVATE | MEXTRA, -1, 0);\n\tif (mem == MAP_FAILED) {\n'
     '\t\trx_ps5_jit_stage("page_alloc_failed_errno", (unsigned long long)errno);\n'
     '\t\tmem = NULL;\n\t}\n'
     '\trx_ps5_jit_stage("page_alloc_return", mem != NULL);')
])
print("Pinned RandomX JIT stage instrumentation applied (PS5 build only)")
