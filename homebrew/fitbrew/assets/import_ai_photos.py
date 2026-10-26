#!/usr/bin/env python3
"""One-time AI photo source import for FITBREW v0.1.3.

Generated FITBREW photos are fetched only by CI on the first source import.
The main-branch release Action commits the 12 resulting pose_*.jpg source
files to GitHub so all subsequent builds use checked-in assets offline.
No PS5 network connection or USB is required.
"""
from pathlib import Path
from io import BytesIO
import hashlib
import urllib.request
from PIL import Image, ImageOps

ASSETS=Path(__file__).resolve().parent
URLS={
  "0": "https://d2jqrm6oza8nb6.cloudfront.net/datasets/a253af68-e038-49d3-9517-e4dfa30cd48c.png?_jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJrZXlIYXNoIjoiYzk3NzczN2M2YzkwMTM3OSIsImJ1Y2tldCI6InJ1bndheS1kYXRhc2V0cyIsInN0YWdlIjoicHJvZCIsImV4cCI6MTc5MzEzOTk2NH0.oj0w3jJc2XknSUUSgVZZAxV4jKMJsFJrU7MiE3TGvqk",
  "1": "https://d2jqrm6oza8nb6.cloudfront.net/datasets/6941d15c-efbf-4564-87d6-7dec93e171d8.png?_jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJrZXlIYXNoIjoiMWNhMDhkMTlkNjJhNGMyMCIsImJ1Y2tldCI6InJ1bndheS1kYXRhc2V0cyIsInN0YWdlIjoicHJvZCIsImV4cCI6MTc5MzEzMTkyOX0.y2e_5fS7dQi_Ad2f_XH2DAHqWDzVxj5uod8jARl1Y4E",
  "2": "https://d2jqrm6oza8nb6.cloudfront.net/datasets/8e98b5e1-7c42-4516-969b-23e20bad1536.png?_jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJrZXlIYXNoIjoiZTJmOGIwZGQ4YWZjYmQ2MCIsImJ1Y2tldCI6InJ1bndheS1kYXRhc2V0cyIsInN0YWdlIjoicHJvZCIsImV4cCI6MTc5MzE4NzQyMX0.qeXoNTkmZjt6a8-95IVr4NKfF5lziIWiMW_eQQJJgQI",
  "3": "https://d2jqrm6oza8nb6.cloudfront.net/datasets/5e55a20b-3109-4744-8c1e-5def142489be.png?_jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJrZXlIYXNoIjoiNDY5NGFhN2JiZDc4ZWYwNSIsImJ1Y2tldCI6InJ1bndheS1kYXRhc2V0cyIsInN0YWdlIjoicHJvZCIsImV4cCI6MTc5MzE4MTY5MX0.vPNC2Cd_A7XWN-n2V4e4f6hn_XO0EG2BiiNLjIgYT_U",
  "4": "https://d2jqrm6oza8nb6.cloudfront.net/datasets/0041cb9f-a8a5-4f52-bfa3-93ffe7905efd.png?_jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJrZXlIYXNoIjoiY2I3ZTM1Mzk4MTFiMmI3YiIsImJ1Y2tldCI6InJ1bndheS1kYXRhc2V0cyIsInN0YWdlIjoicHJvZCIsImV4cCI6MTc5MzE4MjAwOX0.fVh8kFDHSCLhNg1AYT-rfugwgDETsGtW2ONYnYYRExk",
  "5": "https://d2jqrm6oza8nb6.cloudfront.net/datasets/725b0dbc-ddf1-46c7-80b7-ab55572fbe79.png?_jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJrZXlIYXNoIjoiYTE0NzcyYTU1NzUzN2EzOCIsImJ1Y2tldCI6InJ1bndheS1kYXRhc2V0cyIsInN0YWdlIjoicHJvZCIsImV4cCI6MTc5MzE3MDk4MX0.dxaZXU0c8ZpcsTvBvlEKPiHloB5AagTFZusi5F_cFNc",
  "6": "https://d2jqrm6oza8nb6.cloudfront.net/datasets/837ed7c0-790f-4acd-9757-981671134772.png?_jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJrZXlIYXNoIjoiODJkNGU2ZTA1OWVmYzAzMSIsImJ1Y2tldCI6InJ1bndheS1kYXRhc2V0cyIsInN0YWdlIjoicHJvZCIsImV4cCI6MTc5MzE0NDQxNn0.bGK-UekUbuccsdNGEUrlG--7Z3_IWdejNd0461nxqb0",
  "9": "https://d2jqrm6oza8nb6.cloudfront.net/datasets/60f6e1a3-df02-466f-a4bb-00b6de04d017.png?_jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJrZXlIYXNoIjoiYTVhMmQyNDY0MDNkZGQ5MCIsImJ1Y2tldCI6InJ1bndheS1kYXRhc2V0cyIsInN0YWdlIjoicHJvZCIsImV4cCI6MTc5MzE3MzcwMn0.SXzrVocjJiclsfoyJkMG9vALr9E1Xmizcmtl8B36NtE",
  "10": "https://d2jqrm6oza8nb6.cloudfront.net/datasets/056d5806-8143-4eb9-b1ec-d9cbe795e9f7.png?_jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJrZXlIYXNoIjoiZGYzYzQ0YTllYmZmZWYwYSIsImJ1Y2tldCI6InJ1bndheS1kYXRhc2V0cyIsInN0YWdlIjoicHJvZCIsImV4cCI6MTc5MzE0NzMwNX0.O_nhlFNmM-pfXsoSYCt88QXQedNH3Q_B6Q6yKEP8T20",
  "11": "https://d2jqrm6oza8nb6.cloudfront.net/datasets/9b951c02-c56a-4f92-81ee-68ee60f641d4.png?_jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJrZXlIYXNoIjoiYzk5NDViNWFhZDM4MWJhMSIsImJ1Y2tldCI6InJ1bndheS1kYXRhc2V0cyIsInN0YWdlIjoicHJvZCIsImV4cCI6MTc5MzEzMzUzMH0.BYnU-6AnsIbLMvvKmIgb_POfEYafhnyQLBsbQBHfbfQ"
}
MONTAGE="https://d2jqrm6oza8nb6.cloudfront.net/datasets/25364949-da11-42f2-93dd-5faa2f579712.png?_jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJrZXlIYXNoIjoiM2FiNzkzNGIwMDc0MzgyZiIsImJ1Y2tldCI6InJ1bndheS1kYXRhc2V0cyIsInN0YWdlIjoicHJvZCIsImV4cCI6MTc5MzEyMjEyM30.85bYY2gIBdr-IgeLm1-xRxpcfGIWZxjV8C0z_lpezgE"
IMPORTED=[]
def download(url):
    req=urllib.request.Request(url,headers={"User-Agent":"FITBREW-AI-Art-Importer/0.1.3","Accept":"image/png"})
    with urllib.request.urlopen(req,timeout=45) as response:
        if response.status!=200:
            raise RuntimeError("Asset download HTTP "+str(response.status))
        data=response.read(12000000)
    if not 12000<len(data)<12000000:
        raise RuntimeError("Asset invalid byte length "+str(len(data)))
    return Image.open(BytesIO(data)).convert("RGB")

for index in range(12):
    dest=ASSETS/f"pose_{index}.jpg"
    if dest.is_file():
        img=Image.open(dest);img.verify()
        if dest.stat().st_size<12000:raise RuntimeError("Invalid checked-in artwork "+str(dest))
        print("AI_ART_EXISTING",index,dest.stat().st_size,flush=True)
        continue
    if index in (7,8):
        # Two original compositions from the existing generated 4x3 sheet:
        # row 3, columns 3 and 4, respectively. Crop out captions and numerals.
        sheet=download(MONTAGE)
        w,h=sheet.size
        col=2 if index==7 else 3
        x0=int(col*w/4+5);x1=int((col+1)*w/4-5)
        y0=int(2*h/3+5);y1=int(2*h/3+h/3*.70)
        img=sheet.crop((x0,y0,x1,y1))
    else:
        img=download(URLS[str(index)])
    img=ImageOps.fit(img,(512,512),method=Image.Resampling.LANCZOS)
    img.save(dest,format="JPEG",quality=90,optimize=True,subsampling=0)
    assert dest.stat().st_size>12000
    IMPORTED.append(index)
    print("AI_ART_IMPORTED",index,dest.stat().st_size,
          hashlib.sha256(dest.read_bytes()).hexdigest(),flush=True)
assert all((ASSETS/f"pose_{i}.jpg").is_file() for i in range(12))
print("AI_ART_READY 12/12; newly imported",len(IMPORTED),flush=True)
