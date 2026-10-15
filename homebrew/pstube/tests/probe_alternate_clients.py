#!/usr/bin/env python3
"""Validate alternate public playback clients WITHOUT printing signed URLs."""
import json
import urllib.request
import urllib.error
import concurrent.futures
VIDEO="dQw4w9WgXcQ"
CLIENTS=[
 ("visionos","VISIONOS","1.02",101,
  "Mozilla/5.0 (Macintosh; Intel Mac OS X 15_7_3) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/26.0 Safari/605.1.15"),
 ("web_embedded","WEB_EMBEDDED_PLAYER","2.20260708.00.00",56,
  "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.5 Safari/605.1.15"),
 ("android_vr","ANDROID_VR","1.65.10",28,
  "com.google.android.apps.youtube.vr.oculus/1.65.10 (Linux; U; Android 12L) gzip")
]
def run(item):
 name,client,version,cid,ua=item
 ctx={"clientName":client,"clientVersion":version,"hl":"en","gl":"US","userAgent":ua}
 if client=="VISIONOS":ctx.update(deviceMake="Apple",deviceModel="RealityDevice17,1",osName="visionOS",osVersion="26.5.23O471")
 if client=="ANDROID_VR":ctx.update(androidSdkVersion=32,osName="Android",osVersion="12L",deviceMake="Oculus",deviceModel="Quest 3")
 payload={"videoId":VIDEO,"context":{"client":ctx},"playbackContext":{"contentPlaybackContext":{"html5Preference":"HTML5_PREF_WANTS"}}}
 if client=="WEB_EMBEDDED_PLAYER":payload["context"]["thirdParty"]={"embedUrl":"https://www.youtube.com/embed/"+VIDEO}
 request=urllib.request.Request("https://www.youtube.com/youtubei/v1/player?prettyPrint=false",
 data=json.dumps(payload,separators=(',',':')).encode(),
 headers={"User-Agent":ua,"X-YouTube-Client-Name":str(cid),"X-YouTube-Client-Version":version,
 "Origin":"https://www.youtube.com","Content-Type":"application/json","Accept":"application/json"})
 try:
  with urllib.request.urlopen(request,timeout=14) as response:data=json.load(response)
 except Exception as e:return [name,"API_ERR",type(e).__name__,str(e)[:70]]
 playable=(data.get("playabilityStatus") or {}).get("status")
 streams=data.get("streamingData") or {}
 videos=[f for f in streams.get("adaptiveFormats",[]) if
  "video/mp4" in f.get("mimeType","") and "avc1" in f.get("mimeType","") and f.get("height") in (720,1080)]
 audios=[f for f in streams.get("adaptiveFormats",[]) if "audio/mp4" in f.get("mimeType","") and "mp4a." in f.get("mimeType","")]
 output=[name,"status",playable,"hls",bool(streams.get("hlsManifestUrl")),
 "video",[(f.get("itag"),f.get("height"),bool(f.get("url"))) for f in videos],
 "audio",[(f.get("itag"),bool(f.get("url"))) for f in audios]]
 for f in videos[:2]+audios[:1]:
  url=f.get("url")
  if not url:continue
  total=f.get("contentLength")
  print("ALT FORMAT",name,f.get("itag"),"height",f.get("height"),"total",total,flush=True)
  for start,length in ((0,4096),(7737344,4096),(8388608,4096),(12582912,4096)):
   if total and start>=int(total):continue
   try:
    req=urllib.request.Request(url,headers={"User-Agent":ua,"Range":f"bytes={start}-{start+length-1}"})
    with urllib.request.urlopen(req,timeout=8) as response:
     response.read(128)
     print("ALT RANGE",name,f.get("itag"),start,response.status,flush=True)
   except urllib.error.HTTPError as error:print("ALT RANGE",name,f.get("itag"),start,error.code,flush=True)
   except Exception as error:print("ALT RANGE",name,f.get("itag"),start,type(error).__name__,flush=True)
 return output
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
 for output in pool.map(run,CLIENTS):print("ALT PLAYER",*output,flush=True)
