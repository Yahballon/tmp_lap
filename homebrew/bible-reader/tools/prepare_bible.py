#!/usr/bin/env python3
"""Bundle the public-domain World English Bible British Edition into a strict offline TSV.
Network access is build-time only. No Bible text is fetched by the installed app.
Official upstream: https://ebible.org/Scriptures/engwebpb_vpl.zip
"""
import argparse
import hashlib
import io
import pathlib
import re
import urllib.request
import zipfile

SOURCES = [
    "https://ftp.ebible.org/Scriptures/engwebpb_vpl.zip",
    "https://pacificbibles.org/Scriptures/engwebpb_vpl.zip",
    "https://ebible.org/Scriptures/engwebpb_vpl.zip",
]
NAMES = """Genesis|Exodus|Leviticus|Numbers|Deuteronomy|Joshua|Judges|Ruth|1 Samuel|2 Samuel|1 Kings|2 Kings|1 Chronicles|2 Chronicles|Ezra|Nehemiah|Esther|Job|Psalms|Proverbs|Ecclesiastes|Song of Solomon|Isaiah|Jeremiah|Lamentations|Ezekiel|Daniel|Hosea|Joel|Amos|Obadiah|Jonah|Micah|Nahum|Habakkuk|Zephaniah|Haggai|Zechariah|Malachi|Matthew|Mark|Luke|John|Acts|Romans|1 Corinthians|2 Corinthians|Galatians|Ephesians|Philippians|Colossians|1 Thessalonians|2 Thessalonians|1 Timothy|2 Timothy|Titus|Philemon|Hebrews|James|1 Peter|2 Peter|1 John|2 John|3 John|Jude|Revelation""".split("|")
CODES = """Gen Exo Lev Num Deu Jos Jdg Rut 1Sa 2Sa 1Ki 2Ki 1Ch 2Ch Ezr Neh Est Job Psa Pro Ecc Sol Isa Jer Lam Eze Dan Hos Joe Amo Oba Jon Mic Nah Hab Zep Hag Zec Mal Mat Mar Luk Joh Act Rom 1Co 2Co Gal Eph Phi Col 1Th 2Th 1Ti 2Ti Tit Phm Heb Jam 1Pe 2Pe 1Jo 2Jo 3Jo Jud Rev""".split()
ALIASES = {}
def normal(s):
    return re.sub(r"[^a-z0-9]", "", s.lower())
for i, (name, code) in enumerate(zip(NAMES, CODES)):
    ALIASES[normal(name)] = i
    ALIASES[normal(code)] = i
ALIASES.update({normal(k):v for k,v in {
    "Psalm":18,"Ps":18,"Song":21,"Song of Songs":21,"Canticles":21,
    "Mark":40,"1 Kings":10,"Jhn":42,"Jn":42,"1 Cor":45,"1 Tim":53,
    "Ezek":25,"Matt":39,"Gen":0,"Rev":65,"Phil":49,"1 Sam":8
}.items()})
REF = re.compile(r"^\s*(.*?)\s+(\d{1,3}):(\d{1,3})(?:-\d+)?\s+(.+?)\s*$")
def parse_line(line):
    m = REF.match(line.replace("\ufeff", ""))
    if not m:
        return None
    book = ALIASES.get(normal(m[1]))
    if book is None:
        return None
    ch, verse = int(m[2]), int(m[3])
    text = re.sub(r"\s+", " ", m[4].strip())
    if not 1 <= ch <= 150 or not 1 <= verse <= 180 or not text:
        return None
    return (book, ch, verse, text)
def convert_zip(data):
    if len(data)>20_000_000:
        raise ValueError("Source ZIP unexpectedly large")
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        parts=[]
        for item in archive.infolist():
            if item.is_dir() or item.file_size>30_000_000:
                continue
            if pathlib.PurePosixPath(item.filename).suffix.lower() not in (".txt",".vpl"):
                continue
            content=archive.read(item).decode("utf-8-sig",errors="replace")
            parsed=[v for line in content.splitlines() if (v:=parse_line(line)) is not None]
            if len(parsed)>1000:
                parts.extend(parsed)
    unique={}
    for b,c,v,text in parts:
        key=(b,c,v)
        if key in unique and unique[key]!=text:
            raise ValueError(f"Duplicate verse with conflicting text: {key}")
        unique[key]=text
    counts={b: set() for b in range(66)}
    for b,c,v in unique:
        counts[b].add(c)
    if not all(counts.values()) or sum(map(len,counts.values()))!=1189:
        raise ValueError(f"Incomplete Bible: chapters={sum(map(len,counts.values()))}, books={sum(bool(x) for x in counts.values())}")
    if not 30000<=len(unique)<=40000:
        raise ValueError(f"Incomplete Bible: {len(unique)} verses")
    if "in the beginning" not in unique.get((0,1,1),"").lower() or (65,22,21) not in unique:
        raise ValueError("Expected first and last passages missing")
    return "".join(f"{b}\t{c}\t{v}\t{t.replace(chr(9),' ')}\n" for (b,c,v),t in sorted(unique.items()))
def main():
    p=argparse.ArgumentParser()
    p.add_argument("output",type=pathlib.Path)
    p.add_argument("--input",type=pathlib.Path,help="Local ZIP (for deterministic offline test/rebuild)")
    args=p.parse_args()
    if args.input:
        data=args.input.read_bytes()
    else:
        data = None
        for url in SOURCES:
            try:
                req = urllib.request.Request(url, headers={"User-Agent":"Mozilla/5.0", "Accept":"application/zip,*/*"})
                with urllib.request.urlopen(req, timeout=45) as response:
                    data = response.read(20_000_001)
                print("Source:", url)
                break
            except Exception as exc:
                print("Source unavailable:", url, exc)
        if data is None:
            raise RuntimeError("No public-domain Bible source reachable during build")
    print("Upstream ZIP SHA256:",hashlib.sha256(data).hexdigest())
    result=convert_zip(data)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text("# World English Bible British Edition (WEBPB) - public domain, https://ebible.org/\n"+result,encoding="utf-8")
    print(f"Bundled {result.count(chr(10))} offline verses; SHA256:",hashlib.sha256(args.output.read_bytes()).hexdigest())
if __name__=="__main__":
    main()
