#!/usr/bin/env python3
"""Stage original ZASM table for PS5 -fno-exceptions build.

Exactly two compile-time consistency checks use C++ 'throw'. They are never
triggered for the upstream table. For PS5, turn them into compiler traps:
during constexpr evaluation a duplicate now causes a hard compile-time error.
No command/opcode or runtime behavior is substituted. Host builds use the
unmodified GPLv3 upstream file and execute the full command-table tests.
"""
import argparse
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument("staged_cpp", type=Path)
a = p.parse_args()
code = a.staged_cpp.read_text()
replacements = (
    ('throw "Duplicate command found";', '__builtin_trap();'),
    ('throw "Duplicate variable found";', '__builtin_trap();'),
)
assert code.count("throw ") == len(replacements), "Unexpected exceptions in upstream table"
for before, after in replacements:
    assert code.count(before) == 1, "Upstream table changed; investigate before porting"
    code = code.replace(before, after)
assert 'get_script_command(int command)' in code
assert 'get_script_variable(int32_t var)' in code
a.staged_cpp.write_text(code)
print("Staged unmodified opcodes with two exception-free constexpr duplicate guards")
