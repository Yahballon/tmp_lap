#!/usr/bin/env python3
"""Private CI diagnostic: signed URLs never printed, uploaded or persisted."""
import urllib.request,urllib.error
from pathlib import Path
p=Path("/tmp/pstube-hd-stream-urls")
urls=p.read_text().splitlines();p.unlink(missing_ok=True)
assert len(urls)==2
ua_vr="com.google.android.apps.youtube.vr.oculus/1.65.10 (Linux; U; Android 12L) gzip"
ua_web="Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36"
for label,url in zip(("720p H264","AAC"),urls):
 if label=="720p H264":
  offsets=[(0,1048575),(1048576,2097151),(3145728,4194303),(6291456,7340031),
   (7340032,7602175),(7602176,7733247),(7667712,7733247),
   (7733248,7737343),(7737344,7741439),(7737344,7798783),
   (7733248,7798783),(7667712,7798783),(7602176,7864319),
   (7602176,8650751),(7340032,8388607),
   (7737344,7737344),(7737343,7737344),(7737000,7737400),
   (8388608,8392703),(10485760,10489855),(15728640,15732735),
   (20971520,20975615),(26000000,26004095)]
  from urllib.parse import urlsplit,parse_qsl, urlencode,urlunsplit
  parsed=urlsplit(url)
  print("HD URL query keys",sorted(set(k for k,_ in parse_qsl(parsed.query))),flush=True)
  for begin,end in [(7737344,7741439),(7340032,8388607),(8388608,8392703)]:
   adjusted=[(k,v) for k,v in parse_qsl(parsed.query) if k not in ("range","rn","rbuf")]
   adjusted.extend([("range",f"{begin}-{end}"),("rn","1")])
   q=urlunsplit((parsed.scheme,parsed.netloc,parsed.path,urlencode(adjusted),parsed.fragment))
   for hdr in (None,f"bytes={begin}-{end}"):
    try:
     request=urllib.request.Request(q,headers={"User-Agent":ua_vr,
       "Accept":"*/*",**({"Range":hdr} if hdr else {})})
     with urllib.request.urlopen(request,timeout=8) as response:
      response.read(512)
      print("HD QUERY RANGE",begin,end,hdr is not None,response.status,flush=True)
    except urllib.error.HTTPError as ex:print("HD QUERY RANGE",begin,end,hdr is not None,ex.code,flush=True)
    except Exception as ex:print("HD QUERY RANGE",begin,end,hdr is not None,type(ex).__name__,flush=True)
  for begin,end in offsets:
   try:
    req=urllib.request.Request(url,headers={"User-Agent":ua_vr,
         "Accept":"*/*","Range":f"bytes={begin}-{end}"})
    with urllib.request.urlopen(req,timeout=8) as resp:
     resp.read(256)
     print("HD OFFSET DIAG",begin,end,resp.status,flush=True)
   except urllib.error.HTTPError as ex: print("HD OFFSET DIAG",begin,end,ex.code,flush=True)
   except Exception as ex:print("HD OFFSET DIAG",begin,end,type(ex).__name__,flush=True)

 assert url.startswith("https://") and ".googlevideo.com/" in url[:300]
 for user_agent,range_value in [(ua_vr,None),(ua_vr,"bytes=0-4095"),(ua_vr,"bytes=0-16383"),
       (ua_vr,"bytes=0-65535"),(ua_vr,"bytes=0-262143"),
       (ua_vr,"bytes=0-1048575"),
       (ua_vr,"bytes=7340032-8388607"),
       (ua_vr,"bytes=7340032-7602175"),
       (ua_vr,"bytes=7340032-7405567"),
       (ua_vr,"bytes=7340032-7356415"),
       (ua_vr,"bytes=7340032-7344127"),
       (ua_vr,"bytes=0-"),(ua_vr,"bytes=1048576-1052671"),
       (ua_web,None),(ua_web,"bytes=0-4095")]:
  name=("VR" if user_agent==ua_vr else "WEB")+" "+(range_value or "whole GET")
  try:
   req=urllib.request.Request(url,headers={"User-Agent":user_agent,"Accept":"*/*",
                   **({"Range":range_value} if range_value else {})})
   with urllib.request.urlopen(req,timeout=12) as response:
    buf=response.read(4096)
    magic=b"ftyp" in buf[:64] or b"moov" in buf[:64] or b"mdat" in buf[:64]
    print("MEDIA HTTPS:",label,name,"HTTP",response.status,
          "content_type",response.headers.get("Content-Type"),
          "body_prefix_bytes",len(buf),"mp4",magic,
          "range",response.headers.get("Content-Range","")[:75],flush=True)
  except urllib.error.HTTPError as e:
   print("MEDIA HTTPS:",label,name,"HTTP",e.code,flush=True)
  except Exception as e:
   print("MEDIA HTTPS:",label,name,"error",type(e).__name__,flush=True)
