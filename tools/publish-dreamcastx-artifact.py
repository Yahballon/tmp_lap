#!/usr/bin/env python3
"""Publish a verified native PS5DreamcastX ZIP with a PUBLIC raw Git URL."""
import hashlib,json,os,shutil,subprocess,sys,tempfile,time,urllib.request
from datetime import datetime,timezone
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BRANCH="testbuilds-artifacts"
def git(*args,cwd=ROOT,check=True):
    return subprocess.run(["git",*args],cwd=cwd,check=check,text=True,capture_output=True)
def main():
    app=json.loads((ROOT/"homebrew/ps5-dreamcastx/app.json").read_text())
    version=app["version"]
    assert app["titleId"]=="PPSA99765" and app["installFolder"]=="PS5DreamcastX"
    assert app["releaseTag"]==f"ps5-dreamcastx-v{version}"
    runid=os.environ["SOURCE_BUILD_RUN_ID"]
    assert runid.isdigit()
    zip_file=ROOT/"homebrew/ps5-dreamcastx/dist/PPSA99765.zip"
    assert zip_file.is_file() and zip_file.stat().st_size>100000
    size=zip_file.stat().st_size
    sha=hashlib.sha256(zip_file.read_bytes()).hexdigest()
    git("config","user.name","github-actions[bot]")
    git("config","user.email","41898282+github-actions[bot]@users.noreply.github.com")
    with tempfile.TemporaryDirectory(prefix="dreamcastx-public-") as td:
        dest=Path(td)/"public"
        branch=git("ls-remote","--exit-code","--heads","origin",BRANCH,check=False)
        if branch.returncode==0:
            git("fetch","origin",BRANCH)
            git("worktree","add","--detach",str(dest),"FETCH_HEAD")
            git("switch","-c",BRANCH,cwd=dest)
        else:
            git("worktree","add","--detach",str(dest),"HEAD")
            git("switch","--orphan",BRANCH,cwd=dest)
        git("config","user.name","github-actions[bot]",cwd=dest)
        git("config","user.email","41898282+github-actions[bot]@users.noreply.github.com",cwd=dest)
        relative=f"ps5-dreamcastx/v{version}/PPSA99765.zip"
        output=dest/relative
        output.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(zip_file,output)
        git("add","--",relative,cwd=dest)
        if git("diff","--cached","--quiet",cwd=dest,check=False).returncode!=0:
            git("commit","-m",f"verified native PS5DreamcastX v{version}",cwd=dest)
            git("push","origin",f"HEAD:refs/heads/{BRANCH}",cwd=dest)
        git("worktree","remove","--force",str(dest))
    url=f"https://raw.githubusercontent.com/Yahballon/tmp_lap/{BRANCH}/{relative}"
    for attempt in range(8):
        try:
            with urllib.request.urlopen(urllib.request.Request(url,headers={
                    "User-Agent":"PS5DreamcastX-Publisher"}),timeout=120) as resp:
                assert hashlib.sha256(resp.read()).hexdigest()==sha,"Remote ZIP hash mismatch"
            break
        except Exception:
            if attempt==7:raise
            time.sleep(3)
    print("Verified public URL:",url,flush=True)
    data={"title_id":"PPSA99765","version":version,"tag":app["releaseTag"],
          "zip_name":"PPSA99765.zip","zip_url":url,"size_bytes":size,
          "sha256":sha,"build_run_id":int(runid),
          "published_at":datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")}
    path=ROOT/"testbuilds/ps5-dreamcastx.json"
    path.parent.mkdir(exist_ok=True)
    path.write_text(json.dumps(data,indent=2)+"\n")
    subprocess.run([sys.executable,str(ROOT/"tools/update-version.py")],check=True,cwd=ROOT)
    version_data=json.loads((ROOT/"version.json").read_text())
    entry=next(x for x in version_data["apps"] if x["title_id"]=="PPSA99765")
    assert entry["version"]==version and entry["zip_url"]==url,entry
    git("add","--","testbuilds/ps5-dreamcastx.json","version.json")
    if git("diff","--cached","--quiet",check=False).returncode!=0:
        git("commit","-m",f"chore: PS5DreamcastX v{version} public TESTBUILDS package [skip ci]")
        for attempt in range(3):
            git("pull","--rebase","origin","main")
            result=git("push","origin","HEAD:main",check=False)
            if result.returncode==0:break
            if attempt==2:raise RuntimeError(result.stderr)
            time.sleep(3)
    print("PASS: TESTBUILD version.json updated to "+version,flush=True)
if __name__=="__main__":main()
