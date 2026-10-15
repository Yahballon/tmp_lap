#!/usr/bin/env python3
"""Diagnostic only: inspect public watch-page availability; print no signed URLs."""
import concurrent.futures
import json
import re
import urllib.request
import urllib.error

ua="Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36"
id="dQw4w9WgXcQ"
headers={"User-Agent":ua,"Accept":"text/html,application/xhtml+xml,*/*;q=0.8","Accept-Encoding":"identity"}

def probe(path):
    try:
        req=urllib.request.Request(path,headers=headers)
        with urllib.request.urlopen(req,timeout=16) as f:
            html=f.read(6_000_000).decode("utf8","replace")
            print("URL",path[:90],"STATUS",f.status,"LENGTH",len(html),flush=True)
        for key in ('ytInitialPlayerResponse = ','var ytInitialPlayerResponse = ','"streamingData":','"formats":'):
            idx=html.find(key)
            print("KEY",repr(key),idx,flush=True)
        start=html.find("ytInitialPlayerResponse = ")
        if start<0: start=html.find("var ytInitialPlayerResponse = ")
        if start<0:return
        start=html.find("{",start)
        if start<0:return
        decoder=json.JSONDecoder()
        response,_=decoder.raw_decode(html[start:])
        data=response.get('streamingData') or {}
        print("PLAYABILITY",response.get('playabilityStatus',{}).get('status'),"reason",str(response.get('playabilityStatus',{}).get('reason'))[:100],flush=True)
        print("FORMATS",len(data.get('formats') or []),"ADAPTIVE",len(data.get('adaptiveFormats') or []),"HLS",bool(data.get('hlsManifestUrl')),flush=True)
        for fmt in (data.get('formats') or [])[:6]:
            direct=fmt.get("url")
            print("FORMAT",fmt.get('itag'),fmt.get('mimeType'),fmt.get('qualityLabel'),"url",bool(direct),"cipher",bool(fmt.get('signatureCipher')),flush=True)
            if direct:
                request=urllib.request.Request(direct,headers={"User-Agent":ua,"Range":"bytes=0-1023"})
                try:
                    with urllib.request.urlopen(request,timeout=10) as f:
                        beginning=f.read(1024)
                        print("MEDIA PROBE",f.status,f.headers.get("Content-Type"),len(beginning),flush=True)
                except Exception as exc:
                    print("MEDIA PROBE ERROR",type(exc).__name__,str(exc)[:110],flush=True)
    except Exception as exc:
        print("WATCH ERROR",type(exc).__name__,str(exc)[:120],flush=True)

urls=[
    "https://www.youtube.com/watch?v="+id,
    "https://www.youtube.com/embed/"+id,
    "https://www.youtube.com/watch?v=Xy_Cznzdfj0",
]
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as ex:
    list(ex.map(probe,urls))
