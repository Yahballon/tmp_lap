#!/usr/bin/env python3
import hashlib,json,os,re,urllib.request
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
REPO="Yahballon/tmp_lap"
def main():
    titles={"PPSA99442":("Crypto Tracker","crypto-tracker")}
    for path in (ROOT/"homebrew").glob("*/app.json"):
        d=json.loads(path.read_text())
        if d["titleId"] in titles:raise ValueError("Duplicate Title ID")
        titles[d["titleId"]]=(d["name"],path.parent.name)
    latest={}
    for page in range(1,101):
        headers={"Accept":"application/vnd.github+json","User-Agent":"testbuilds-catalog"}
        if os.getenv("GH_TOKEN"):headers["Authorization"]="Bearer "+os.environ["GH_TOKEN"]
        req=urllib.request.Request("https://api.github.com/repos/"+REPO+"/releases?per_page=100&page="+str(page),headers=headers)
        with urllib.request.urlopen(req,timeout=30) as response: releases=json.load(response)
        if not isinstance(releases,list):raise ValueError("Invalid GitHub API response")
        for release in releases:
            if release.get("draft"):continue
            tag=release.get("tag_name","")
            # Relayfall is deliberately published as an experimental TESTBUILD.
            # Opt in only this title; other apps retain stable-only selection.
            if release.get("prerelease") and not re.fullmatch(r"starbound-rebased-v\d+\.\d+\.\d+",tag):continue
            m=re.search(r"v(\d+)\.(\d+)\.(\d+)(?:-[A-Za-z0-9][A-Za-z0-9.-]*)?$",tag)
            if not m:continue
            semver=tuple(map(int,m.groups()))
            for asset in release.get("assets",[]):
                filename=asset.get("name","")
                if not re.fullmatch(r"PPSA\d{5}\.zip",filename):continue
                title=filename[:-4]
                if title not in titles or asset.get("state")!="uploaded" or not asset.get("size"):continue
                label,slug=titles[title]
                if title=="PPSA99442":
                    if not (re.fullmatch(r"v\d+\.\d+\.\d+",tag) or tag.startswith("crypto-tracker-v")):continue
                elif not tag.startswith(slug+"-v"):continue
                url=asset.get("browser_download_url")
                if not url:continue
                entry={"name":label,"slug":slug,"title_id":title,"version":".".join(m.groups()),"tag":tag,"zip_name":filename,"zip_url":url,"release_url":release["html_url"],"source":"homebrew/"+slug,"size_bytes":asset["size"],"published_at":release.get("published_at")}
                old=latest.get(title)
                if old is None or (semver,entry["published_at"] or "")>(old[0],old[1]["published_at"] or ""):latest[title]=(semver,entry)
        if len(releases)<100:break
    else:raise ValueError("Release pagination limit exceeded")
    # Public signed TESTBUILD fallback for DreamcastX. GitHub's Actions token
    # can upload build artifacts but is currently denied Release API writes
    # (HTTP 403). The binary is committed to a separate public git branch;
    # never point a console downloader at authenticated Actions artifacts.
    fallback=ROOT/"testbuilds/ps5-dreamcastx.json"
    if fallback.is_file():
        d=json.loads(fallback.read_text())
        app=json.loads((ROOT/"homebrew/ps5-dreamcastx/app.json").read_text())
        title="PPSA99765"
        version=d.get("version","")
        tag="ps5-dreamcastx-v"+version
        url=("https://raw.githubusercontent.com/"+REPO+
             "/testbuilds-artifacts/ps5-dreamcastx/v"+version+"/PPSA99765.zip")
        valid=(app["titleId"]==title and title in titles and
               re.fullmatch(r"\d+\.\d+\.\d+",version) is not None and
               app["version"]==version and app["releaseTag"]==tag and
               d.get("title_id")==title and d.get("tag")==tag and
               d.get("zip_name")=="PPSA99765.zip" and
               d.get("zip_url")==url and
               isinstance(d.get("size_bytes"),int) and d["size_bytes"]>100000 and
               re.fullmatch(r"[a-f0-9]{64}",d.get("sha256","")) is not None and
               re.fullmatch(r"\d{4}-\d\d-\d\dT\d\d:\d\d:\d\dZ",d.get("published_at","")) is not None)
        if not valid:
            raise ValueError("Invalid DreamcastX signed-artifact fallback metadata")
        semver=tuple(map(int,version.split(".")))
        entry={"name":titles[title][0],"slug":"ps5-dreamcastx",
               "title_id":title,"version":version,"tag":tag,
               "zip_name":"PPSA99765.zip","zip_url":url,
               "release_url":"https://github.com/"+REPO+
                              "/actions/runs/"+str(d["build_run_id"]),
               "source":"homebrew/ps5-dreamcastx",
               "size_bytes":d["size_bytes"],"published_at":d["published_at"]}
        old=latest.get(title)
        if old is None or semver>old[0]:
            latest[title]=(semver,entry)
    if not latest:raise ValueError("No valid releases; refusing empty catalog")
    apps=[latest[key][1] for key in sorted(latest)]
    checksum=hashlib.sha256(json.dumps(apps,sort_keys=True,separators=(",",":")).encode()).hexdigest()
    manifest={"schema":2,"version":checksum,"repository":REPO,"app_count":len(apps),"apps":apps}
    output=json.dumps(manifest,indent=2,ensure_ascii=False)+"\n"
    path=ROOT/"version.json"
    if not path.exists() or path.read_text()!=output:
        path.write_text(output);print("Updated manifest:",len(apps),"apps")
    else:print("Manifest unchanged")
if __name__=="__main__":main()
