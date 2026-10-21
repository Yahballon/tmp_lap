#!/usr/bin/env python3
"""Offline test of the real Godot-to-PS5DevBridge linker staging."""
import importlib.util
import json
from pathlib import Path
import tempfile

path=Path(__file__).resolve().parents[1]/"scripts/stage_native_link.py"
spec=importlib.util.spec_from_file_location("godot_link",path)
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

with tempfile.TemporaryDirectory() as d:
    root=Path(d)
    gd=root/"godot"
    app=root/"app"
    (app/"tools").mkdir(parents=True)
    (app/"sce_sys").mkdir()
    entry=root/"main.cpp"
    entry.write_text('extern "C" int godot_ps5_run_pack(const char*);')
    (app/"sce_sys/param.json").write_text(json.dumps({"localizedParameters":{"en-US":{}}}))
    (app/"tools/build.sh").write_text(
      'link_inputs=("$build/obj/app_crt.o" "$build/obj/app_cpp_runtime.o" "${objects[@]}")\n'
      'for archive in "${archives[@]}"; do\n'
      '    [[ $archive =~ ^[A-Za-z0-9_.-]+(/[A-Za-z0-9_.-]+)*\\.a$ && -f $root/$archive ]] || exit 2\n'
      '    link_inputs+=("$root/$archive")\n'
      'done\n')
    files=[]
    for i in range(52):
        name="libcore" if i==0 else "libmain" if i==1 else "libmodule_gdscript" if i==2 else "libother"+str(i)
        location=("bin/obj/core/" if i==0 else "bin/obj/main/" if i==1 else
                  "bin/obj/modules/" if i==2 else "bin/obj/misc/")
        rel=location+name+".ps5.template_debug.x86_64.a"
        f=gd/rel
        f.parent.mkdir(parents=True,exist_ok=True)
        f.write_bytes(b"!<arch>\n"+b"X"*128)
        files.append({"path":rel,"size_bytes":f.stat().st_size})
    (gd/"godot-ps5-full-manifest.json").write_text(json.dumps({
        "format":"godot-ps5-static-libs", "target":"x86_64-sie-ps5",
        "archive_count":len(files), "archives":files
    }))
    sdk = app/".deps/native/ps5-payload-sdk/target/lib"
    sdk.mkdir(parents=True)
    for lib in ("libc++.a", "libc++abi.a", "libunwind.a"):
        (sdk/lib).write_bytes(b"!<arch>\n"+b"Q"*128)
    builtins=root/"libclang_rt.builtins-x86_64.a"
    builtins.write_bytes(b"!<arch>\n"+b"R"*128)
    assert module.stage(gd,app,entry,builtins_override=builtins) is None
    b=(app/"tools/build.sh").read_text()
    assert '--start-group' in b and '--end-group' in b
    assert '[A-Za-z0-9_.+-]+' in b, "PS5 libc++.a filenames rejected by old regex"
    assert 'APP_STATIC_ARCHIVES :=' in (app/"GNUmakefile").read_text()
    assert 'libmodule_gdscript' in (app/"GNUmakefile").read_text()
    assert (app/"godot-libs"/files[0]["path"]).is_file()
    assert (app/"godot-libs/runtime/libc++abi.a").is_file()
    assert json.loads((app/"sce_sys/param.json").read_text())["titleId"]=="PPSA99843"
    assert (app/"src/godot-smoke/main.cpp").is_file()
print("PASS: All 52 Godot libraries staged for grouped headless PS5 link")
