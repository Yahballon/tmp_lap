// SPDX-License-Identifier: MIT
#include <cstdio>
#include <sys/stat.h>
extern "C" int godot_ps5_run_pack(const char *);
static void log_message(const char *message) {
    if (FILE *f = fopen("/data/GodotPS5/engine-smoke.log","ab")) {
        fprintf(f, "[GodotPS5 engine] %s\n", message);
        fclose(f);
    }
}
// PS5 CRT's catchReturnFromMain is intentionally a weak hook. Its default
// returns and then calls libc exit(status), causing SIGSYS on real PS5 titles.
// Request that the PS5 shell close this homebrew instead, then wait for it.
extern "C" int sceSystemServiceLoadExec(const char *path, const char *const *argv);
extern "C" int sceKernelUsleep(unsigned int microseconds);
extern "C" void catchReturnFromMain(int status) {
    char message[96]{};
    std::snprintf(message, sizeof(message), "SHELL_EXIT_REQUEST v0.2.8 status=%d", status);
    log_message(message);
    std::fflush(nullptr);
    (void)sceSystemServiceLoadExec("exit", nullptr);
    for (;;) {
        (void)sceKernelUsleep(100000);
    }
}
int main() {
    mkdir("/data/GodotPS5",0755);
    log_message("MAIN_ENTER_BEFORE_REDIRECT v0.2.8");
    FILE *out=freopen("/data/GodotPS5/engine-stdout.log","ab",stdout);
    FILE *err=freopen("/data/GodotPS5/engine-stderr.log","ab",stderr);
    if(out) setvbuf(stdout,nullptr,_IONBF,0);
    if(err) setvbuf(stderr,nullptr,_IONBF,0);
    log_message("NATIVE_GODOT_ENTRY_START v0.2.8 ENGINE_PHASE_TRACE");
    log_message("PACK=/app0/assets/godot/smoke.pck");
    log_message("ENGINE_PHASE_TRACE=PROFILER_OS_MAIN_SETUP");
    const int result=godot_ps5_run_pack("/app0/assets/godot/smoke.pck");
    char msg[96]{};
    snprintf(msg,sizeof(msg),"NATIVE_GODOT_RETURN=%d",result);
    log_message(msg);
    return result;
}
