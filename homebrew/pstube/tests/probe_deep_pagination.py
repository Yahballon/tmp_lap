#!/usr/bin/env python3
"""Diagnostic: identify why YouTube search pagination ends after 20-40 results.
Prints only structural metadata, never continuation tokens or cookies.
"""
import json,urllib.request,re,time,collections
URL="https://www.youtube.com"
UA="Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36"
def fetch(path,body=None,version=""):
    headers={"User-Agent":UA,"Accept":"text/html,application/json;q=0.9,*/*;q=0.8","Accept-Encoding":"identity"}
    if body:
        headers.update({"Content-Type":"application/json","X-YouTube-Client-Name":"1","X-YouTube-Client-Version":version,"Origin":URL})
    request=urllib.request.Request(URL+path,data=(json.dumps(body).encode() if body else None),headers=headers)
    with urllib.request.urlopen(request,timeout=16) as response:return response.read(5_000_000).decode("utf-8","replace")
def config(html,key):
    m=re.search(r'"'+re.escape(key)+r'":"([^"]+)"',html)
    return m.group(1) if m else ""
def collect(item,path="",out=None):
    if out is None:out=[]
    if isinstance(item,dict):
        if "continuationCommand" in item:
            command=item["continuationCommand"]
            if isinstance(command,dict) and isinstance(command.get("token"),str):
                out.append((path+"/continuationCommand",command["token"]))
        if "nextContinuationData" in item and isinstance(item["nextContinuationData"],dict):
            token=item["nextContinuationData"].get("continuation")
            if isinstance(token,str):out.append((path+"/nextContinuationData",token))
        for k,v in item.items():collect(v,path+"/"+k,out)
    elif isinstance(item,list):
        for index,value in enumerate(item):collect(value,path+"/"+str(index),out)
    return out
def parse_initial(source):
    needle="var ytInitialData = "
    if needle not in source:needle="ytInitialData = "
    start=source.find(needle)
    if start<0:raise RuntimeError("No initial data")
    start=source.find("{",start)
    obj,end=json.JSONDecoder().raw_decode(source[start:])
    return obj
source=fetch("/results?search_query=PS5")
version=config(source,"INNERTUBE_CONTEXT_CLIENT_VERSION") or "2.20261010.00.00"
key=config(source,"INNERTUBE_API_KEY")
visitor=config(source,"VISITOR_DATA")
print("CONFIG",bool(key),bool(version),bool(visitor),"html",len(source),flush=True)
initial=parse_initial(source)
tokens=collect(initial)
print("PAGE 1 video_renderers",source.count('"videoRenderer":{'),"tokens",[(p,len(t)) for p,t in tokens[:12]],flush=True)
if not tokens:raise RuntimeError("No continuation token in initial")
scroll_tokens=[t for path,t in tokens if "continuationItemRenderer" in path]
if not scroll_tokens:raise RuntimeError("No scroll continuation in initial search")
token=scroll_tokens[-1]
seen=set()
for page in range(2,9):
    if token in seen:
        print("LOOP at page",page,flush=True);break
    seen.add(token)
    body={"context":{"client":{"clientName":"WEB","clientVersion":version,"hl":"en","gl":"US",**({"visitorData":visitor} if visitor else {})}},"continuation":token}
    path="/youtubei/v1/search?prettyPrint=false"+("&key="+key if key else "")
    try:
        raw=fetch(path,body,version)
        data=json.loads(raw)
    except Exception as exc:
        print("PAGE",page,"ERROR",type(exc).__name__,str(exc)[:150],flush=True);break
    toks=collect(data)
    locations=collections.Counter(p.rsplit("/",2)[-2] if "/" in p else p for p,_ in toks)
    print("PAGE",page,"bytes",len(raw),"videos",raw.count('"videoRenderer":{'),
          "tokens",[(p,len(t)) for p,t in toks[:5]],"continuation_types",dict(locations),
          "keys",list(data.keys()),flush=True)
    scroll=[t for path,t in toks if "continuationItemRenderer" in path]
    if not scroll:
        print("NO SCROLL CONTINUATION",page,flush=True)
        break
    token=scroll[-1]
