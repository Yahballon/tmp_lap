#!/usr/bin/env python3
"""Discover MotionBGS public 1080p MP4 links for ThemeChanger.

The catalog contains only metadata + original source links. Video and artwork
are NOT mirrored or bundled. Users download selected assets privately onto PS5.
"""
from html.parser import HTMLParser
from urllib.parse import urljoin,urlparse
from urllib.request import Request,urlopen
from concurrent.futures import ThreadPoolExecutor,as_completed
import html, re, pathlib, time

ORIGIN="https://motionbgs.com"
UA="Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 Chrome/125 Safari/537.36"
CATEGORIES=[
  ("Featured","/"),("Anime","/tag:anime/"),("Games","/tag:games/"),
  ("Nature","/tag:nature/"),("Cars","/tag:car/"),("Space","/tag:space/"),
  ("Technology","/tag:technology/"),("Fantasy","/tag:fantasy/"),
]
SKIP={"about","privacy-policy","dmca","contact","windows-11","mac","mobile","guides"}
class Anchors(HTMLParser):
    def __init__(self): super().__init__(convert_charrefs=True);self.found=[];self.link=None
    def handle_starttag(self,tag,attrs):
        attrs=dict(attrs)
        if tag=="a":
            self.link=[attrs.get("href") or "",attrs.get("title") or "",""]
        if tag=="img" and self.link:
            self.link[2]+=(attrs.get("alt") or "")+" "
    def handle_data(self,data):
        if self.link:self.link[2]+=data
    def handle_endtag(self,tag):
        if tag=="a" and self.link:
            self.found.append(self.link)
            self.link=None

def request(url,limit=3*1024*1024):
    req=Request(url,headers={"User-Agent":UA,"Accept":"text/html,*/*;q=0.8","Accept-Encoding":"identity"})
    with urlopen(req,timeout=18) as res:
        if urlparse(res.geturl()).hostname!="motionbgs.com":raise RuntimeError("redirect outside source")
        return res.read(limit+1)[:limit].decode("utf-8","replace")

def clean(s):
    return re.sub(r"\s+"," ",html.unescape(re.sub(r"<[^>]*>"," ",s))).strip()

def links(category,path):
    h=request(ORIGIN+path)
    p=Anchors();p.feed(h)
    found=[]
    for href,title,text in p.found:
        url=urljoin(ORIGIN+"/",href)
        parsed=urlparse(url)
        slug=parsed.path.strip("/")
        if parsed.hostname!="motionbgs.com" or not re.fullmatch(r"[a-z0-9][a-z0-9-]{2,100}",slug) or slug in SKIP:continue
        name=clean(title or text)
        if not 3<=len(name)<=100:continue
        if name.lower().startswith(("more ","next","previous","how to")):continue
        found.append((slug,name.replace(" Live Wallpaper","").replace(" 4K",""),category))
    return found

def page(item):
    slug,name,category=item
    try:
        raw=request(f"{ORIGIN}/{slug}",limit=600000)
        hit=re.search(r'(?:href|data-href)=["\'](?:https?://motionbgs\.com)?(/dl/hd/(\d+))["\']',raw,re.I)
        if not hit:
            hit=re.search(r'(/dl/hd/(\d+))',raw,re.I)
        if not hit:return None
        fid=hit.group(2)
        title=re.search(r'<h1[^>]*>(.*?)</h1>',raw,re.I|re.S)
        if title:
            better=clean(title.group(1)).removesuffix(" Live Wallpaper")
            if 3<len(better)<90:name=better
        image=""
        for rex in [
            r'<meta[^>]+property=["\']og:image["\'][^>]+content=["\']([^"\']+)',
            r'<meta[^>]+content=["\']([^"\']+)["\'][^>]+property=["\']og:image',
            r'<meta[^>]+name=["\']twitter:image["\'][^>]+content=["\']([^"\']+)',
        ]:
            m=re.search(rex,raw,re.I)
            if m:
                candidate=urljoin(ORIGIN,html.unescape(m.group(1)))
                if urlparse(candidate).scheme=="https" and len(candidate)<420:
                    image=candidate
                break
        return (slug,name[:90],category,f"{ORIGIN}/dl/hd/{fid}",image)
    except Exception as e:
        print("Skip:",slug,type(e).__name__,str(e)[:100],flush=True)
        return None

def main():
    collected=[]
    for category,path in CATEGORIES:
        try:
            found=links(category,path)
            print(category,"candidates",len(found),flush=True)
            collected.extend(found[:22])
        except Exception as e:
            print("Category unavailable:",category,str(e)[:120],flush=True)
    # Safe fallback: download IDs verified from the site's public HTML links.
    collected += [("color-bridge","Color Bridge","Nature"),
                  ("steam","Steam","Games")]
    unique={}
    for item in collected:
        if item[0] not in unique:unique[item[0]]=item
    records=[]
    with ThreadPoolExecutor(max_workers=6) as pool:
        jobs=[pool.submit(page,x) for x in list(unique.values())[:155]]
        for job in as_completed(jobs):
            record=job.result()
            if record:records.append(record)
    records.sort(key=lambda v:(next((i for i,x in enumerate(CATEGORIES) if x[0]==v[2]),20),v[1].lower()))
    if len(records)<6:
        raise SystemExit("MotionBGS catalog discovery failed: fewer than 6 confirmed HD MP4 entries")
    output=pathlib.Path(__file__).resolve().parents[1]/"generated/motionbgs/catalog.tsv"
    output.parent.mkdir(parents=True,exist_ok=True)
    with output.open("w",encoding="utf-8") as f:
        f.write("# slug\ttitle\tcategory\tmp4_url\tpreview_url\n")
        for values in records:
            f.write("\t".join(str(v).replace("\t"," ").replace("\n"," ") for v in values)+"\n")
    print("Live verified MotionBGS 1080p catalog:",len(records),"records",flush=True)
    for r in records[:6]:print("  ",r[:4],flush=True)

if __name__=="__main__":main()
