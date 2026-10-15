#!/usr/bin/env python3
"""Live diagnostic of search continuation; no auth and no private credentials."""
import concurrent.futures,json,re,urllib.request,urllib.error,html
UA="Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36"
search="https://www.youtube.com/results?search_query=PS5"
req=urllib.request.Request(search,headers={"User-Agent":UA,"Accept":"text/html,*/*","Accept-Encoding":"identity"})
with urllib.request.urlopen(req,timeout=16) as response: page=response.read(5_000_000).decode("utf8","replace")
def tokens(s):
    # Avoid leaking actual continuation tokens into logs.
    patterns=[r'"continuationCommand":\{"token":"([^"]+)"',r'"nextContinuationData":\{"continuation":"([^"]+)"']
    values=[]
    for p in patterns:
        for found in re.findall(p,s):
            try: found=json.loads('"'+found+'"')
            except: pass
            if found and found not in values:values.append(found)
    return values
found=tokens(page)
print("INITIAL",len(page),"videos",len(re.findall(r'"videoRenderer":\{',page)),"continuations",len(found),flush=True)
for n in ('INNERTUBE_API_KEY','INNERTUBE_CONTEXT_CLIENT_VERSION','VISITOR_DATA'):
  m=re.search(r'"'+n+r'":"([^"]+)"',page)
  print("CLIENT_FIELD",n,"found",bool(m),flush=True)
key=re.search(r'"INNERTUBE_API_KEY":"([^"]+)"',page)
vers=re.search(r'"INNERTUBE_CONTEXT_CLIENT_VERSION":"([^"]+)"',page)
visitor=re.search(r'"VISITOR_DATA":"([^"]+)"',page)
version=vers.group(1) if vers else "2.20261010.00.00"
if not found:raise SystemExit("No YouTube continuation in initial search page")
for idx,token in enumerate(found[-3:]):
  body=json.dumps({"context":{"client":{"clientName":"WEB","clientVersion":version,"hl":"en","gl":"US",
      **({"visitorData":visitor.group(1)} if visitor else {})}},"continuation":token}).encode()
  endpoint="https://www.youtube.com/youtubei/v1/search?prettyPrint=false"
  if key:endpoint+="&key="+key.group(1)
  req=urllib.request.Request(endpoint,data=body,headers={"User-Agent":UA,
       "Content-Type":"application/json","Accept":"application/json",
       "X-YouTube-Client-Name":"1","X-YouTube-Client-Version":version,
       "Origin":"https://www.youtube.com"},method="POST")
  try:
    with urllib.request.urlopen(req,timeout=15) as response: raw=response.read(5_000_000).decode("utf8","replace")
    print("CONTINUATION",idx,"HTTP 200","bytes",len(raw),"videos",len(re.findall(r'"videoRenderer":\{',raw)),
          "next",len(tokens(raw)),"request token length",len(token),flush=True)
    if '"videoRenderer":{' in raw:
       print("PAGINATION_SUCCESS",idx,flush=True)
       break
  except Exception as e:print("CONTINUATION",idx,"failed",type(e).__name__,str(e)[:100],flush=True)
