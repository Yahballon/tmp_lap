#!/usr/bin/env python3
"""Instrument the REAL PS5 DevBridge CRT and original ZQuest zplayer main.

Diagnostic v0.0.2, not a replacement launcher. All entries go to
/data/ZQuestEngineBoot/logs/boot.log, signal events to crash.log.
The CRT log starts AFTER _init_env, before _init / C++ static constructors.
On fatal SIGSEGV/SIGBUS/SIGILL/SIGABRT, preserve the original OS crash
by restoring SIG_DFL and re-raising after synchronous file writes.

No heap allocations in logging. Static initializer callbacks are not replaced,
but we skip null array entries which would otherwise cause RIP=0.
"""
from pathlib import Path
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--crt", required=True, type=Path)
parser.add_argument("--zelda", required=True, type=Path)
args = parser.parse_args()

crt = args.crt.read_text()

def once(text, before, after, label):
    assert text.count(before) == 1, f"CRT/engine drift: {label} ({text.count(before)})"
    return text.replace(before, after, 1)

logger = r'''
// PS5 engine boot-test v0.0.2: persistent early logger.
// Opens only after the native CRT's _init_env, before static constructors.
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <cstdio>
#include <cstdint>
namespace {
int zc_boot_fd = -1;
int zc_crash_fd = -1;
const char* zc_boot_path = "/data/ZQuestEngineBoot/logs/boot.log";
const char* zc_crash_path = "/data/ZQuestEngineBoot/logs/crash.log";
size_t zc_strlen(const char* s) {
    if (!s) return 0;
    size_t n=0; while (n<512 && s[n]) ++n; return n;
}
void zc_rawwrite(int fd, const char* s) {
    if (fd >= 0 && s) (void)::write(fd, s, zc_strlen(s));
}
void zc_signal_log(int signum) {
    const char* label = signum == SIGSEGV ? "SIGSEGV" :
                        signum == SIGBUS ? "SIGBUS" :
                        signum == SIGILL ? "SIGILL" :
                        signum == SIGABRT ? "SIGABRT" : "OTHER_SIGNAL";
    zc_rawwrite(zc_crash_fd, "PPSA99128 ENGINE CRASH SIGNAL ");
    zc_rawwrite(zc_crash_fd, label);
    zc_rawwrite(zc_crash_fd, "\n");
    if (zc_crash_fd >= 0) (void)::fsync(zc_crash_fd);
    zc_rawwrite(zc_boot_fd, "FATAL_SIGNAL=");
    zc_rawwrite(zc_boot_fd, label);
    zc_rawwrite(zc_boot_fd, "\n");
    if (zc_boot_fd >= 0) (void)::fsync(zc_boot_fd);
    ::signal(signum, SIG_DFL);
    ::raise(signum); // keep native OS crash reporting
    ::_exit(128 + signum);
}
}
extern "C" void zc_engine_boot_mark(const char* stage) {
    if (zc_boot_fd < 0) return;
    zc_rawwrite(zc_boot_fd, "[ZQuest zplayer PS5 boot v0.0.2] ");
    zc_rawwrite(zc_boot_fd, stage);
    zc_rawwrite(zc_boot_fd, "\n");
    (void)::fsync(zc_boot_fd);
}
extern "C" void zc_engine_bootlog_init() {
    (void)::mkdir("/data/ZQuestEngineBoot", 0755);
    (void)::mkdir("/data/ZQuestEngineBoot/logs", 0755);
    zc_boot_fd = ::open(zc_boot_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    zc_crash_fd = ::open(zc_crash_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    // Existing v0.1.5 browser data path as fallback if a separate directory
    // is unavailable in a sandboxed homebrew launch.
    if (zc_boot_fd < 0) {
        (void)::mkdir("/data/ZQuestClassic", 0755);
        (void)::mkdir("/data/ZQuestClassic/logs", 0755);
        zc_boot_fd = ::open("/data/ZQuestClassic/logs/engine-boot.log",
                            O_CREAT | O_WRONLY | O_APPEND, 0644);
    }
    if (zc_crash_fd < 0)
        zc_crash_fd = ::open("/data/ZQuestClassic/logs/engine-crash.log",
                              O_CREAT | O_WRONLY | O_APPEND, 0644);
    zc_engine_boot_mark("BOOT_LOG_READY_AFTER_INIT_ENV");
    ::signal(SIGSEGV, zc_signal_log);
    ::signal(SIGBUS, zc_signal_log);
    ::signal(SIGILL, zc_signal_log);
    ::signal(SIGABRT, zc_signal_log);
}
'''
assert '#include <pthread.h>' in crt
crt = once(crt, '#include <pthread.h>', '#include <pthread.h>\n' + logger, 'logger definition')

crt = once(crt, '    _init_env(process_parameters);',
    '    _init_env(process_parameters);\n    zc_engine_bootlog_init();', 'post-libc start')

crt = once(crt, '    _init();',
    '    zc_engine_boot_mark("BEFORE_STATIC_CONSTRUCTORS");\n'
    '    _init();\n'
    '    zc_engine_boot_mark("AFTER_STATIC_CONSTRUCTORS");', 'ctor stage')

crt = once(crt, '    MainArguments args{argc, argv, 1};',
    '    zc_engine_boot_mark("BEFORE_APP_THREAD_CREATE");\n'
    '    MainArguments args{argc, argv, 1};', 'thread create')

crt = once(crt, '    rc = pthread_join(thread, nullptr);',
    '    zc_engine_boot_mark("APP_THREAD_CREATED");\n'
    '    rc = pthread_join(thread, nullptr);', 'thread join')

crt = once(crt, '    catchReturnFromMain(status);',
    '    zc_engine_boot_mark("APP_THREAD_JOINED");\n'
    '    catchReturnFromMain(status);', 'after main return')

crt = once(crt, '    args->status = application_main(args->argc, args->argv, nullptr);',
    '    zc_engine_boot_mark("APP_THREAD_ENTER_BEFORE_MAIN");\n'
    '    args->status = application_main(args->argc, args->argv, nullptr);\n'
    '    zc_engine_boot_mark("APP_THREAD_MAIN_RETURNED");', 'application main')

# Explicitly record each invoked constructor address and index so the last
# logged record identifies the static initializer that faulted. Calling NULL
# constructor array padding causes RIP=0 on some PS5 toolchains; ignore these.
crt = once(crt, '    while (first != last)\n        (*first++)();',
    '''    unsigned index = 0;
    while (first != last) {
        const Initializer fn = *first++;
        char line[128];
        std::snprintf(line, sizeof(line), "STATIC_CTOR index=%u fn=%p",
                      index++, reinterpret_cast<const void*>(fn));
        zc_engine_boot_mark(line);
        if (fn != nullptr) fn();
        else zc_engine_boot_mark("STATIC_CTOR_NULL_ENTRY_SKIPPED");
    }''', 'run_forward ctor')

args.crt.write_text(crt)

zelda = args.zelda.read_text()
zelda = once(zelda, 'int main(int argc, char **argv)\n{',
    'extern "C" void zc_engine_boot_mark(const char*);\n'
    'int main(int argc, char **argv)\n{\n'
    '\tzc_engine_boot_mark("ZPLAYER_MAIN_ENTER");', 'zelda main')
for call, name in (
    ('\tzalleg_setup_allegro(App::zelda, argc, argv);', 'ALLEGRO_SETUP'),
    ('\tallocate_crap();', 'INITIAL_BITMAP_ALLOCATIONS'),
    ('\tFFCore.clear_combo_scripts();\n\n\tZ_title', 'FFCORE_SCRIPTS_CLEAR'),
    ('\tif(!get_qst_buffers())', 'QUEST_BUFFERS'),
    ('\tload_control_schemes();', 'LOAD_CONTROLS'),
    ('\tzplayer_handle_commands();', 'HANDLE_COMMANDS'),
    ('\tinit_and_run_main_zplayer_loop();', 'GAME_LOOP'),
):
    zelda = once(zelda, call,
        '\tzc_engine_boot_mark("BEFORE_' + name + '");\n' + call, name)
# Enter the original game loop, before invoking graphics and save subsystems.
zelda = once(zelda,
    'void init_and_run_main_zplayer_loop()\n{',
    'void init_and_run_main_zplayer_loop()\n{\n'
    '\tzc_engine_boot_mark("ORIGINAL_PLAYER_LOOP_ENTER");',
    'player loop entry')
args.zelda.write_text(zelda)
print("PS5 BOOTLOG INSTRUMENTED: early CRT, constructors, engine main and game-loop")
