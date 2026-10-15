#!/usr/bin/env python3
"""Cross-compile genuine MesenCE-Libretro for PS5. No synthetic eboot."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed
import os, subprocess
root=Path(__file__).resolve().parents[1]
src=root/"build/MesenCE-Libretro"
sdk=Path(os.environ["PS5_PAYLOAD_SDK"])
wrapper=root/"upstream/PS5SNESX/ps5/proto/native/tooling/prospero-clang18"
out=root/"build/ps5-core"
assert sdk.joinpath("target/include").exists() and wrapper.is_file()
cpp=[src/"Libretro/libretro.cpp",src/"Core/pch.cpp"]
for part in ("Core/NES","Core/Shared","Core/Netplay","Utilities"):
    cpp+=list((src/part).rglob("*.cpp"))
cpp+=[p for p in (src/"Core/Debugger").rglob("*.cpp") if not p.name.startswith("ExpressionEvaluator.") or p.name=="ExpressionEvaluator.cpp"]
cpp+=[src/"Core/Debugger/ExpressionEvaluator.Nes.cpp"]
c=list((src/"SevenZip").rglob("*.c"))+list((src/"Utilities").rglob("*.c"))
c += [p for p in (src/"Lua").rglob("*.c") if p.name not in {"serial.c","unix.c","unixdgram.c","unixstream.c"}]
sources=sorted(set(c+cpp))
assert len(sources)>100 and all(p.is_file() for p in sources)
common=["-O1","-fPIC","-march=znver2","-ffunction-sections","-fdata-sections","-DLIBRETRO"]
common += ["-I"+str(src/p) for p in (".","Core","Utilities","Libretro","Core/Shared","Core/NES")]
def build(p):
    obj=out/p.relative_to(src).with_suffix(".o")
    obj.parent.mkdir(parents=True,exist_ok=True)
    if p.suffix==".cpp":
        cmd=["sh",str(wrapper),*common,"-std=c++17","-fexceptions","-frtti","-Wno-undefined-inline"]
    else:
        cmd=["clang-18","-target","x86_64-sie-ps5","-isysroot",str(sdk),
             "-isystem",str(sdk/"target/include"),*common,"-std=gnu11"]
    run=subprocess.run([*cmd,"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
    return p,obj,run.returncode,run.stderr[-3200:]
print("Cross-building",len(sources),"native MesenCE objects",flush=True)
failure=0
with ThreadPoolExecutor(max_workers=4) as ex:
    jobs=[ex.submit(build,p) for p in sources]
    for task in as_completed(jobs):
        p,obj,status,errors=task.result()
        if status:
            failure+=1
            print("FAILED",p.relative_to(src),errors,flush=True)
            if failure>=8:break
if failure:raise SystemExit(f"{failure} PS5 core object(s) failed to compile")
archive=out/"libmesen_ce_nes.ps5.a"
subprocess.run(["llvm-ar-18","rcs",str(archive),*(str(out/p.relative_to(src).with_suffix(".o")) for p in sources)],check=True)
assert archive.stat().st_size>100_000
print("PASS real PS5 MesenCE archive:",archive,flush=True)
