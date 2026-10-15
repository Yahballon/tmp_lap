#!/usr/bin/env python3
"""Probe anonymous public YouTube player clients for actual separate HD AVC/AAC.
No login, no user data, no cookie exports, no signed URLs printed.
"""
import concurrent.futures,json,re,urllib.request,urllib.error,ssl
IDS=["dQw4w9WgXcQ","jNQXAC9IVRw"]
UA="Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36"
CLIENTS=[
("VISIONOS","VISIONOS","1.02","com.google.ios.youtube/21.26.4 (iPhone; iOS 18.3)",None),
("ANDROID_VR","ANDROID_VR","1.65.10","com.google.android.apps.youtube.vr.oculus/1.65.10 (Linux; U; Android 12L) gzip",None),
("WEB_EMBEDDED","WEB_EMBEDDED_PLAYER","1.20261010.00.00",UA,"embed"),
("WEB_SAFARI","WEB","2.20261010.00.00","Mozilla/5.0 (Macintosh; Intel Mac OS X 15_7) AppleWebKit/605.1.15 Version/26.0 Safari/605.1.15",None),
("IOS","IOS","21.26.4","com.google.ios.youtube/21.26.4 (iPhone16,2; U; CPU iOS 18_3_2 like Mac OS X;)",None),
]
ids={x[1]:x[0] for x in CLIENTS}
def req(url, headers=None,body=None):
 r=urllib.request.Request(url,headers=headers or {"User-Agent":UA},data=body)
 with urllib.request.urlopen(r,timeout=13,context=ssl.create_default_context()) as f:return f.read(4_000_000)
def analyze(conf):
 name,client,version,ua,embed,vid,guest=conf
 data={"videoId":vid,"context":{"client":{"clientName":client,"clientVersion":version,"hl":"en","gl":"US",
   "userAgent":ua}},"playbackContext":{"contentPlaybackContext":{"html5Preference":"HTML5_PREF_WANTS"}}}
 if client=="ANDROID_VR":data["context"]["client"].update({"androidSdkVersion":32,"osName":"Android","osVersion":"12L","deviceMake":"Oculus","deviceModel":"Quest 3"})
 if client=="VISIONOS":data["context"]["client"].update({"deviceMake":"Apple","deviceModel":"RealityDevice17,1","osName":"visionOS","osVersion":"26.5.23O471"})
 if client=="IOS":data["context"]["client"].update({"deviceMake":"Apple","deviceModel":"iPhone16,2","osName":"iOS","osVersion":"18.3"})
 if embed:data["context"]["thirdParty"]={"embedUrl":"https://www.youtube.com/embed/"+vid}
 if guest:data["context"]["client"]["visitorData"]=guest
 cid={"ANDROID_VR":"28","VISIONOS":"31","WEB_EMBEDDED_PLAYER":"56","WEB":"1","IOS":"5"}.get(client,"1")
 try:
  j=json.loads(req("https://www.youtube.com/youtubei/v1/player?prettyPrint=false",{
   "Content-Type":"application/json","Accept":"application/json","User-Agent":ua,
   "X-YouTube-Client-Name":cid,"X-YouTube-Client-Version":version,"Origin":"https://www.youtube.com"},
   json.dumps(data).encode()))
  st=(j.get("playabilityStatus") or {}).get("status")
  reason=str((j.get("playabilityStatus") or {}).get("reason",""))[:80]
  stream=j.get("streamingData") or {}
  hd=[(v.get("itag"),v.get("height"),bool(v.get("url"))) for v in (stream.get("adaptiveFormats") or []) if
      "video/mp4" in v.get("mimeType","") and "avc1" in v.get("mimeType","") and (v.get("height") or 0)>=720]
  aac=[(v.get("itag"),bool(v.get("url"))) for v in (stream.get("adaptiveFormats") or []) if "audio/mp4" in v.get("mimeType","") and "mp4a" in v.get("mimeType","")]
  return f"{name} {vid} visitor={bool(guest)} status={st} reason={reason} hd={hd[:5]} aac={aac[:3]} hls={bool(stream.get('hlsManifestUrl'))}"
 except Exception as e:return f"{name} {vid} visitor={bool(guest)} ERROR {type(e).__name__} {str(e)[:100]}"
visitor=""
try:
 text=req("https://www.youtube.com/results?search_query=PS5",{"User-Agent":UA,"Accept":"text/html,*/*","Accept-Encoding":"identity"}).decode("utf8","replace")
 found=re.search(r'"VISITOR_DATA":"([^"]+)"',text)
 if found:visitor=found.group(1)
except Exception as ex:print("GUEST_SESSION",type(ex).__name__,str(ex)[:70])
print("GUEST_VISITOR_AVAILABLE",bool(visitor))
work=[(*c,v,g) for c in CLIENTS for v in IDS for g in dict.fromkeys(["",visitor]) if (g=="" or bool(g))]
with concurrent.futures.ThreadPoolExecutor(max_workers=8) as ex:
 for result in ex.map(analyze,work):print("GUEST_HD:",result,flush=True)

# Official unauthenticated guest visitor ID endpoint. A YouTube search page's
# visitor value is not necessarily a valid playback guest context.
for vid in ("dQw4w9WgXcQ","jNQXAC9IVRw"):
 for client in ("VISIONOS","ANDROID_VR"):
  version="1.02" if client=="VISIONOS" else "1.65.10"
  agent="Mozilla/5.0 (Macintosh; Intel Mac OS X 15_7_3) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/26.0 Safari/605.1.15" if client=="VISIONOS" else "com.google.android.apps.youtube.vr.oculus/1.65.10 (Linux; U; Android 12L) gzip"
  try:
   guest=json.loads(req("https://youtubei.googleapis.com/youtubei/v1/visitor_id",
        {"User-Agent":agent,"Content-Type":"application/json","Accept":"application/json"},
        json.dumps({"context":{"client":{"clientName":client,"clientVersion":version}}}).encode()))
   token=(guest.get("responseContext") or {}).get("visitorData","")
   print("GUEST_VISITOR_API:",client,"ok",bool(token),flush=True)
   for host in ("www.youtube.com","youtubei.googleapis.com"):
    data={"videoId":vid,"context":{"client":{"clientName":client,"clientVersion":version,
        "visitorData":token,"hl":"en","gl":"US"}},"contentCheckOk":True,"racyCheckOk":True}
    if client=="VISIONOS":data["context"]["client"].update({"deviceMake":"Apple","deviceModel":"RealityDevice17,1","osName":"visionOS","osVersion":"26.5.23O471"})
    if client=="ANDROID_VR":data["context"]["client"].update({"androidSdkVersion":32,"osName":"Android","osVersion":"12L","deviceMake":"Oculus","deviceModel":"Quest 3"})
    j=json.loads(req("https://"+host+"/youtubei/v1/player",{"Content-Type":"application/json","User-Agent":agent},
        json.dumps(data).encode()))
    status=(j.get("playabilityStatus") or {}).get("status")
    hd=[x.get("height") for x in (j.get("streamingData") or {}).get("adaptiveFormats",[]) if
        x.get("url") and x.get("height",0)>=720 and "avc1" in x.get("mimeType","")]
    print("GUEST_ID_RESULT:",client,vid,host,status,"hd",hd[:4],flush=True)
  except Exception as exc:
   print("GUEST_ID_ERROR:",client,vid,type(exc).__name__,str(exc)[:100],flush=True)

# Representative PS5 jailbreak news search clips: measure whether guest
# context improves HD availability on real, non-test public videos.
try:
 page=req("https://www.youtube.com/results?search_query=PS5+JAILBREAK+NEWS",
       {"User-Agent":UA,"Accept":"text/html,*/*","Accept-Encoding":"identity"}).decode("utf8","replace")
 found=list(dict.fromkeys(re.findall(r'"videoRenderer":\\{"videoId":"([A-Za-z0-9_-]{11})"',page)))
 if not found:found=list(dict.fromkeys(re.findall(r'"videoId":"([A-Za-z0-9_-]{11})"',page)))
 found=found[:8]
 print("NEWS_SAMPLE_SIZE:",len(found),flush=True)
 conf=("VISIONOS","VISIONOS","1.02",
   "Mozilla/5.0 (Macintosh; Intel Mac OS X 15_7_3) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/26.0 Safari/605.1.15",None)
 guest=json.loads(req("https://youtubei.googleapis.com/youtubei/v1/visitor_id",
   {"Content-Type":"application/json","User-Agent":conf[3]},
   json.dumps({"context":{"client":{"clientName":"VISIONOS","clientVersion":"1.02"}}}).encode()))
 visitor=(guest.get("responseContext") or {}).get("visitorData","")
 results=[analyze((*conf,vid,visitor)) for vid in found]
 print("NEWS_RESULTS:",*results,sep="\n",flush=True)
except Exception as exc:print("NEWS_SAMPLE_ERROR",type(exc).__name__,str(exc)[:80],flush=True)
