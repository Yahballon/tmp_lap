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
            if release.get("draft") or release.get("prerelease"):continue
            tag=release.get("tag_name","")
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
