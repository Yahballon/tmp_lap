#!/usr/bin/env python3
"""Debug public HD track availability; never print signed media links or credentials."""
import json,urllib.request,urllib.error,concurrent.futures,re
ids=["dQw4w9WgXcQ","jNQXAC9IVRw"]
configs=[
 ("android","ANDROID","21.26.364","3","com.google.android.youtube/21.26.364 (Linux; U; Android 11) gzip"),
 ("android_tv","ANDROID_VR","1.65.10","28","com.google.android.apps.youtube.vr.oculus/1.65.10 (Linux; U; Android 12L) gzip"),
 ("ios","IOS","21.26.4","5","com.google.ios.youtube/21.26.4 (iPhone16,2; U; CPU iOS 18_3_2 like Mac OS X;)"),
 ("tv","TVHTML5","7.20261010.00.00","7","Mozilla/5.0 (ChromiumStylePlatform) Cobalt/Version"),
 ("web","WEB","2.20261014.01.00","1","Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 Chrome/125.0.0.0 Safari/537.36"),
]
def probe(q):
 name,client,version,cid,ua,vid=q
 data={"videoId":vid,"context":{"client":{"clientName":client,"clientVersion":version,"hl":"en","gl":"US"}},"playbackContext":{"contentPlaybackContext":{"html5Preference":"HTML5_PREF_WANTS"}}}
 if client=="ANDROID":
  data["context"]["client"].update({"androidSdkVersion":30,"osName":"Android","osVersion":"11","userAgent":ua})
 if client=="ANDROID_VR":
  data["context"]["client"].update({"androidSdkVersion":32,"osName":"Android","osVersion":"12L","userAgent":ua,"deviceMake":"Oculus","deviceModel":"Quest 3"})
 req=urllib.request.Request("https://www.youtube.com/youtubei/v1/player?prettyPrint=false",data=json.dumps(data,separators=(',',':')).encode(),headers={"User-Agent":ua,"Content-Type":"application/json","Accept":"application/json","X-YouTube-Client-Name":cid,"X-YouTube-Client-Version":version,"Origin":"https://www.youtube.com"})
 try:
  with urllib.request.urlopen(req,timeout=15) as res:j=json.load(res)
 except Exception as ex:return f"{name}/{vid}: request failed {type(ex).__name__}: {str(ex)[:55]}"
 play=j.get("playabilityStatus",{}).get("status")
 sd=j.get("streamingData",{})
 formats=sd.get("formats") or []
 adaptive=sd.get("adaptiveFormats") or []
 def fmt(x):
  mime=x.get("mimeType","")
  codec=mime.split('codecs=')[-1][:45] if 'codecs=' in mime else mime
  return (x.get("itag"),x.get("height"),str(x.get("qualityLabel"))[:9],codec,bool(x.get("url")),bool(x.get("signatureCipher") or x.get("cipher")))
 mux=[fmt(x) for x in formats if "video/mp4" in x.get("mimeType","")]
 h264=[fmt(x) for x in adaptive if "video/mp4" in x.get("mimeType","") and "avc1" in x.get("mimeType","")]
 aac=[fmt(x) for x in adaptive if "audio/mp4" in x.get("mimeType","") and "mp4a" in x.get("mimeType","")]
 other=[fmt(x) for x in adaptive if "video/" in x.get("mimeType","") and "avc1" not in x.get("mimeType","")]
 mediaChecks=[]
 if name=="android_tv" and play=="OK":
  for itag in (136,137,140):
   fmtitem=next((x for x in adaptive if x.get("itag")==itag and x.get("url")),None)
   if not fmtitem:mediaChecks.append(f"{itag}: unavailable");continue
   try:
    url=fmtitem["url"]
    assert url.startswith("https://") and "googlevideo.com" in url.split("/")[2]
    req=urllib.request.Request(url,headers={"User-Agent":ua,"Range":"bytes=0-4095","Accept":"*/*"})
    with urllib.request.urlopen(req,timeout=13) as media:
     start=media.read(4096)
     mediaChecks.append(f"{itag}: HTTP={media.status} bytes={len(start)} mp4={b'ftyp' in start[:64]} mime={media.headers.get('Content-Type','')[:50]}")
   except Exception as e:mediaChecks.append(f"{itag}: {type(e).__name__} {str(e)[:55]}")
 return f"{name}/{vid} status={play} mux={mux[:4]} avc={h264[:10]} aac={aac[:5]} other={other[:6]} adaptiveCount={len(adaptive)} checked={mediaChecks}"
with concurrent.futures.ThreadPoolExecutor(max_workers=5) as ex:
 for result in ex.map(probe,[(a,b,c,d,e,vid) for a,b,c,d,e in configs for vid in ids]):
  print("HD_PROBE:",result,flush=True)
