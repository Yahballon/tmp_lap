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
int main() {
    mkdir("/data/GodotPS5",0755);
    FILE *out=freopen("/data/GodotPS5/engine-stdout.log","ab",stdout);
    FILE *err=freopen("/data/GodotPS5/engine-stderr.log","ab",stderr);
    if(out) setvbuf(stdout,nullptr,_IONBF,0);
    if(err) setvbuf(stderr,nullptr,_IONBF,0);
    log_message("NATIVE_GODOT_ENTRY_START v0.2.1 RX_SEGMENT_TEST");
    log_message("PACK=/app0/assets/godot/smoke.pck");
    log_message("ELF_TEXT_SEGMENT=READ_EXECUTE");
    const int result=godot_ps5_run_pack("/app0/assets/godot/smoke.pck");
    char msg[96]{};
    snprintf(msg,sizeof(msg),"NATIVE_GODOT_RETURN=%d",result);
    log_message(msg);
    return result;
}
