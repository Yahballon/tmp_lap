#!/usr/bin/env python3
"""Stage REAL PS5-DevBridge CRT for original ZQuestClassic zplayer.

The official DevBridge app_crt.cpp has an optional, weak jb_startup_mark
breadcrumb logger. PS5-DevBridge's production app supplies this symbol, but
the upstream ZQuest executable does not, and the native SIE module converter
cannot resolve a weak undefined application import from public SDK stubs.

For the separate ZQuest engine test ONLY, remove exactly those optional log
calls and the weak declaration from a COPY of the proven CRT. Preserve _start,
_init_env, constructor/destructor handling, the 16-MiB application thread,
the real main(argc, argv) call, and native exit/error paths unchanged.

No dummy symbol, unresolved-symbol suppression or fake gameplay is added.
"""
import argparse
from pathlib import Path
import re

parser = argparse.ArgumentParser()
parser.add_argument("original", type=Path)
parser.add_argument("staged_copy", type=Path)
args = parser.parse_args()
assert args.original.is_file(), "Real PS5-DevBridge CRT missing"
assert args.staged_copy != args.original, "Never patch actual DevBridge CRT in place"
source = args.original.read_text()
declaration = '    void jb_startup_mark(const char*) __attribute__((weak));\n'
assert source.count(declaration) == 1, "Pinned DevBridge weak callback changed"
assert 'int application_main(int argc, char **argv, char **envp) __asm__("main");' in source
assert 'void _init_env(void *process_parameters);' in source
assert 'void\n_start(void *process_parameters, Destructor loader_teardown)' in source
assert '_init_env(process_parameters);' in source
assert '_init();' in source and '(void)atexit(_fini);' in source
assert 'args->status = application_main(args->argc, args->argv, nullptr);' in source
assert 'pthread_attr_setstacksize(&attr, 16 * 1024 * 1024)' in source
assert 'pthread_join(thread, nullptr)' in source

pattern = re.compile(
    r'if\s*\(\s*jb_startup_mark\s*\)\s*'
    r'jb_startup_mark\s*\(\s*"v27:[^"]*"\s*\)\s*;'
)
source = source.replace(declaration, "", 1)
source, count = pattern.subn("", source)
assert count == 7, f"Unexpected startup marker count: {count}"
assert "jb_startup_mark" not in source
assert '_init_env(process_parameters);' in source
assert 'void\n_start(void *process_parameters, Destructor loader_teardown)' in source
assert 'args->status = application_main(args->argc, args->argv, nullptr);' in source
args.staged_copy.parent.mkdir(parents=True, exist_ok=True)
args.staged_copy.write_text(source)
print("REAL_PS5_APP_CRT=STAGED_NO_OPTIONAL_WEAK_STARTUP_LOG; native boot semantics preserved")
