#!/usr/bin/env python3
"""Independent diagnostic for native on-PS5 Innertube endpoint candidates.

Only tests access and media magic bytes; never logs credential-bearing signed URLs.
"""
import json,urllib.request,urllib.error,urllib.parse, concurrent.futures,re,time
videos=["dQw4w9WgXcQ","jNQXAC9IVRw"]
clients=[
    ("android","ANDROID","21.26.364","3","com.google.android.youtube/21.26.364 (Linux; U; Android 11) gzip"),
    ("android_vr","ANDROID_VR","1.65.10","28","com.google.android.apps.youtube.vr.oculus/1.65.10 (Linux; U; Android 12L) gzip"),
    ("ios","IOS","21.26.4","5","com.google.ios.youtube/21.26.4 (iPhone16,2; U; CPU iOS 18_3_2 like Mac OS X;)"),
    ("tv","TVHTML5","7.20261010.00.00","7","Mozilla/5.0 (ChromiumStylePlatform) Cobalt/Version"),
    ("web_embedded","WEB_EMBEDDED_PLAYER","1.20261010.00.00","56","Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/125.0.0.0 Safari/537.36")
]
def run(req):
    name,client,version,client_id,agent,vid=req
    payload={"videoId":vid,"context":{"client":{"clientName":client,"clientVersion":version,"hl":"en","gl":"US"}},"playbackContext":{"contentPlaybackContext":{"html5Preference":"HTML5_PREF_WANTS"}}}
    if client=="ANDROID":
        payload["context"]["client"].update({"androidSdkVersion":30,"osName":"Android","osVersion":"11","userAgent":agent})
    if client=="ANDROID_VR":
        payload["context"]["client"].update({"androidSdkVersion":32,"osName":"Android","osVersion":"12L","userAgent":agent,"deviceMake":"Oculus","deviceModel":"Quest 3"})
    if client=="WEB_EMBEDDED_PLAYER":payload["context"]["thirdParty"]={"embedUrl":"https://www.youtube.com/"}
    req=urllib.request.Request("https://www.youtube.com/youtubei/v1/player?prettyPrint=false",
         data=json.dumps(payload).encode(),headers={"Content-Type":"application/json","User-Agent":agent,
          "X-YouTube-Client-Name":client_id,"X-YouTube-Client-Version":version,
          "Origin":"https://www.youtube.com","Accept":"application/json"},method="POST")
    try:
        with urllib.request.urlopen(req,timeout=12) as resp:j=json.load(resp)
        formats=(j.get("streamingData") or {}).get("formats") or []
        play=j.get("playabilityStatus") or {}
        matches=[(x.get("itag"),x.get("qualityLabel"),x.get("url"),bool(x.get("signatureCipher"))) for x in formats if "video/mp4" in x.get("mimeType","")]
        found=[(itag,q,url) for itag,q,url,cipher in matches if url]
        output=f"{name} {vid}: {play.get('status')} {len(matches)} muxed formats {len(found)} direct URLs"
        if found:
            itag,q,url=found[0]
            try:
                req=urllib.request.Request(url,headers={"User-Agent":agent,"Range":"bytes=0-4095"})
                with urllib.request.urlopen(req,timeout=10) as resp:
                    b=resp.read(4096)
                    output+=f"; itag {itag}, media HTTP {resp.status}, bytes {len(b)}, mp4_magic={b[4:8]==b'ftyp' or b'ftyp' in b[:32]}"
            except Exception as e:output+=f"; media rejected {type(e).__name__}: {str(e)[:64]}"
        return output
    except Exception as e:return f"{name} {vid}: {type(e).__name__}: {str(e)[:100]}"
with concurrent.futures.ThreadPoolExecutor(max_workers=5) as ex:
    for line in ex.map(run,[(a,b,c,d,e,v) for a,b,c,d,e in clients for v in videos]):
        print("INNERTUBE CHECK:",line,flush=True)
