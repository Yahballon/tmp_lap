#!/usr/bin/env python3
"""Stage all Godot PS5 static libraries for a real DevBridge linker test."""
import argparse
import json
import shutil
from pathlib import Path

def stage(godot, app, entry, builtins_override=None):
    godot = Path(godot).resolve()
    app = Path(app).resolve()
    manifest = json.loads((godot/"godot-ps5-full-manifest.json").read_text())
    assert manifest["archive_count"] >= 8  # minimal real GDScript smoke engine
    assert manifest["format"] == "godot-ps5-static-libs"
    assert manifest["target"] == "x86_64-sie-ps5"
    libraries = []
    for record in manifest["archives"]:
        rel = Path(record["path"])
        assert not rel.is_absolute() and ".." not in rel.parts
        source = godot/rel
        assert source.is_file() and source.stat().st_size == record["size_bytes"]
        dest = app/"godot-libs"/rel
        dest.parent.mkdir(parents=True,exist_ok=True)
        dest.symlink_to(source)
        libraries.append(dest.relative_to(app).as_posix())
    # The Godot engine uses RTTI and libc++; DevBridge's simple-app
    # template only links a minimal homebrew C++ allocation shim.
    # Bring in the actual PS5 SDK's static C++ ABI and unwind libraries.
    runtime = app/".deps/native/ps5-payload-sdk/target/lib"
    for name in ("libc++.a", "libc++abi.a", "libunwind.a", "libc.a"):
        source = runtime/name
        if not source.is_file():
            raise ValueError("Missing PS5 C++ runtime archive: "+str(source))
        dest = app/"godot-libs/runtime"/name
        dest.parent.mkdir(parents=True,exist_ok=True)
        dest.symlink_to(source)
        libraries.append(dest.relative_to(app).as_posix())
    # SDK libc++/libc++abi static objects contain LLVM dependent-library
    # references to "pthread". On PS5 pthread symbols are provided by the
    # system/runtime libraries, not a standalone libpthread.a file. Supply
    # a named EMPTY archive for dependency discovery; it implements nothing.
    # Undefined pthread symbols must still resolve normally or the link fails.
    sdk_pthread = runtime/"libpthread.a"
    compat_pthread = app/"godot-libs/runtime/libpthread.a"
    if sdk_pthread.is_file():
        compat_pthread.symlink_to(sdk_pthread)
    else:
        compat_pthread.write_bytes(b"!<arch>\n")
    # Builtins are compiler-version dependent, so use Clang's actual
    # resource directory rather than assuming an SDK-specific location.
    import shutil as _shutil
    import subprocess
    compiler = _shutil.which("clang-18") or _shutil.which("clang")
    if not compiler:
        raise ValueError("Missing PS5 Clang for builtins")
    resource = Path(subprocess.check_output([compiler, "--print-resource-dir"],text=True).strip())
    builtin = Path(builtins_override) if builtins_override else resource/"lib/linux/libclang_rt.builtins-x86_64.a"
    if not builtin.is_file():
        raise ValueError("Missing compiler builtins archive: "+str(builtin))
    dest=app/"godot-libs/runtime/libclang_rt.builtins-x86_64.a"
    dest.symlink_to(builtin)
    libraries.append(dest.relative_to(app).as_posix())
    build = app/"tools/build.sh"
    source = build.read_text()
    prefix = 'link_inputs=("$build/obj/app_crt.o" "$build/obj/app_cpp_runtime.o" "${objects[@]}")'
    suffix = '    link_inputs+=("$root/$archive")'
    assert source.count(prefix) == 1 and source.count(suffix) == 1
    # Resolve LLVM !dependent-library "pthread" embedded in libc++ objects.
    # The archive is only a linker alias, not a pthread implementation.
    source = source.replace(prefix, prefix+'\nlink_inputs+=("-L$root/godot-libs/runtime")', 1)
    source = source.replace(prefix,prefix+'\nlink_inputs+=("--start-group")',1)
    source = source.replace(suffix+'\ndone',suffix+'\ndone\nlink_inputs+=("--end-group")',1)
    # DevBridge's original whitelist disallows '+' in static archive names.
    # PS5 libc++.a and libc++abi.a are standard SDK filenames, not invalid.
    # Keep strict path matching while permitting literal plus signs.
    # Only update the static archive validation rule. The same character
    # class also occurs in source paths, import stubs, app metadata, etc.
    original_archive_rule = "    [[ $archive =~ ^[A-Za-z0-9_.-]+(/[A-Za-z0-9_.-]+)*"
    patched_archive_rule = "    [[ $archive =~ ^[A-Za-z0-9_.+-]+(/[A-Za-z0-9_.+-]+)*"
    if source.count(original_archive_rule) != 1:
        raise ValueError("DevBridge static archive validation rule changed")
    source = source.replace(original_archive_rule, patched_archive_rule, 1)
    # The DevBridge converter needs optional Zstd tracing callbacks
    # resolved before converting ELF into SELF. It already excludes
    # decompression trace callbacks when the public SDK lacks exports;
    # apply exactly the same policy to compression instrumentation.
    trace_list = "for name in ['ZSTD_trace_decompress_begin','ZSTD_trace_decompress_end',"
    expanded = "for name in ['ZSTD_trace_compress_begin','ZSTD_trace_compress_end','ZSTD_trace_decompress_begin','ZSTD_trace_decompress_end',"
    if source.count(trace_list) != 1:
        raise ValueError("DevBridge Zstd trace import allowlist changed")
    source = source.replace(trace_list, expanded, 1)
    build.write_text(source)
    # PS5-DevBridge's CRT references an optional weak jb_startup_mark() logger.
    # The PS5 native converter rejects undefined imports not provided by the
    # public SDK; this diagnostic callback is *not* an SDK export. Remove
    # only the optional logger in this temporary engine-test CRT copy, never
    # synthesize an unresolved PS5 import or change libc initialization.
    import re
    crt_file = app/"tooling/native/app_crt.cpp"
    crt = crt_file.read_text()
    declaration = '    void jb_startup_mark(const char*) __attribute__((weak));'
    if crt.count(declaration) != 1:
        raise ValueError("DevBridge optional CRT logger declaration changed")
    call_pattern = r'if\s*\(\s*jb_startup_mark\s*\)\s*jb_startup_mark\("[^"\n]*"\);'
    crt, removed = re.subn(call_pattern, "", crt)
    if removed < 4 or "jb_startup_mark" not in declaration:
        raise ValueError("DevBridge optional CRT logger calls changed")
    crt = crt.replace(declaration, "", 1)
    if "jb_startup_mark" in crt:
        raise ValueError("PS5 CRT still contains non-SDK startup symbol")
    crt_file.write_text(crt)
    print("Excluded", removed, "optional jb_startup_mark diagnostics from PS5 CRT")
    target=app/"src/godot-smoke"
    target.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(entry,target/"main.cpp")
    metadata=app/"sce_sys/param.json"
    p=json.loads(metadata.read_text())
    p.update(titleId="PPSA99843",contentId="UP9000-PPSA99843_00-GODOTPS5HEADLESS",conceptId="99843",contentVersion="01.000.001")
    p["localizedParameters"]["en-US"]["titleName"]="Godot PS5 Engine Smoke"
    metadata.write_text(json.dumps(p,indent=2)+'\n')
    (app/"GNUmakefile").write_text('APP_SOURCE_DIR := src/godot-smoke\nAPP_ASSETS := assets\nAPP_STATIC_ARCHIVES := '+' '.join(libraries)+'\ninclude Makefile\n')
    print("Staged",manifest["archive_count"],"Godot engine archives and 4 PS5 runtime archives")

if __name__=="__main__":
    a=argparse.ArgumentParser()
    a.add_argument("--godot",required=True)
    a.add_argument("--app",required=True)
    a.add_argument("--entry",required=True)
    a.add_argument("--builtins-override")
    x=a.parse_args()
    stage(x.godot,x.app,x.entry,x.builtins_override)
