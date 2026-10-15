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
 assert url.startswith("https://") and ".googlevideo.com/" in url[:300]
 for user_agent,range_value in [(ua_vr,None),(ua_vr,"bytes=0-4095"),(ua_vr,"bytes=0-16383"),
       (ua_vr,"bytes=0-65535"),(ua_vr,"bytes=0-262143"),
       (ua_vr,"bytes=0-1048575"),
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
          "body_prefix_bytes",len(buf),"mp4",magic,flush=True)
  except urllib.error.HTTPError as e:
   print("MEDIA HTTPS:",label,name,"HTTP",e.code,flush=True)
  except Exception as e:
   print("MEDIA HTTPS:",label,name,"error",type(e).__name__,flush=True)
