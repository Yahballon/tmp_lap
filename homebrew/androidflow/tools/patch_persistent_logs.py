#!/usr/bin/env python3
"""Patch pinned Hydroid to log reliably to a writable, FTP-visible data path.

The upstream /app0/android.log fopen silently returns NULL if app0 is
read-only on a jailbreak loader. Keep original path as fallback, and preserve
the separate low-overhead live log on TCP 9025. No external services used.
"""
from pathlib import Path
import argparse

LOG_DIR = "/data/AndroidFlow/logs"
LOG_PATH = LOG_DIR + "/android.log"
CRASH_PATH = LOG_DIR + "/crash.txt"

def exactly_one(code: str, old: str, new: str, path: Path) -> str:
    if code.count(old) != 1:
        raise RuntimeError(f"Source drift in {path}: expected one {old[:80]!r}, found {code.count(old)}")
    return code.replace(old, new)

def patch(source: Path) -> None:
    app = source / "src/app_main.c"
    arm = source / "src/arm_cpu.cpp"

    text = app.read_text()
    text = exactly_one(text,
        '    log_file = fopen(LOG_PATH, "a");',
        '    // AndroidFlow: prefer a stable writable data folder over /app0\n'
        '    // (some PS5 loaders mount /app0 read-only).\n'
        '    int log_dir_ok = 1;\n'
        '    if (mkdir("/data/AndroidFlow", 0777) != 0 && errno != EEXIST) log_dir_ok = 0;\n'
        '    if (log_dir_ok && mkdir("/data/AndroidFlow/logs", 0777) != 0 && errno != EEXIST) log_dir_ok = 0;\n'
        '    const char *active_log = LOG_PATH;\n'
        '    log_file = log_dir_ok ? fopen("/data/AndroidFlow/logs/android.log", "a") : NULL;\n'
        '    if (log_file) active_log = "/data/AndroidFlow/logs/android.log";\n'
        '    if (!log_file) log_file = fopen(LOG_PATH, "a");\n'
        '    if (log_file) {\n'
        '        // Ensure short crash-adjacent logs reach the filesystem promptly.\n'
        '        setvbuf(log_file, NULL, _IOLBF, 0);\n'
        '        say(0, "androidflow: persistent log file %s", active_log);\n'
        '    } else {\n'
        '        say(0, "androidflow: FILE LOG UNAVAILABLE; /data and /app0 both rejected writes (errno=%d). Use live TCP 9025", errno);\n'
        '    }', app)
    app.write_text(text)

    cpp = arm.read_text()
    cpp = exactly_one(cpp,
        '    if (fd == -2) fd = open(APP0 "/trace.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);',
        '    if (fd == -2) {\n'
        '        // A crash handler writes here directly, even if android.log is delayed.\n'
        '        // This folder is initialized by app_main before launching a game.\n'
        '        fd = open("/data/AndroidFlow/logs/crash.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);\n'
        '        if (fd < 0) fd = open(APP0 "/trace.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);\n'
        '    }', arm)
    arm.write_text(cpp)
    print("AndroidFlow persistent logger: /data/AndroidFlow/logs/android.log + crash.txt; APP0 fallback")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    args = parser.parse_args()
    patch(args.source.resolve())

if __name__ == "__main__":
    main()
