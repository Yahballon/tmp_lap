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

# Each corpus is the actual, independently translated public-domain Bible,
# never machine-translated verses. All downloads occur at build time only.
TRANSLATIONS = {
    "en": ("engwebpb", "World English Bible British Edition"),
    "de": ("deu1912", "Lutherbibel 1912"),
    "fr": ("fraLSG", "Louis Segond 1910"),
    "es": ("spaRV1909", "Reina-Valera 1909"),
}
def sources(code):
    identity = TRANSLATIONS[code][0]
    return [f"https://{host}/Scriptures/{identity}_vpl.zip"
            for host in ("ebible.org", "pacificbibles.org", "ftp.ebible.org")]
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
    "Ezek":25,"Matt":39,"Gen":0,"Rev":65,"Phil":49,"1 Sam":8,
    "1 Mose":0,"2 Mose":1,"3 Mose":2,"4 Mose":3,"5 Mose":4,
    "Genèse":0,"Exode":1,"Génesis":0,"Éxodo":1,"Ésaïe":22,
    "1Cor":45,"2Cor":46,"1Th":51,"2Th":52,"1Tim":53,"2Tim":54
}.items()})
# Our baked Inter UI font lacks Unicode smart punctuation. Preserve Bible wording
# while converting typographic punctuation to visually equivalent ASCII glyphs.
# Never use Unicode decoding with 'replace': invalid source data should fail.
DISPLAY_PUNCTUATION = str.maketrans({
    "\u2018": "'", "\u2019": "'", "\u201a": "'", "\u201b": "'",
    "\u02bc": "'", "\u2032": "'",
    "\u201c": '"', "\u201d": '"', "\u201e": '"', "\u201f": '"',
    "\u00ab": '"', "\u00bb": '"',
    "\u2010": "-", "\u2011": "-", "\u2012": "-", "\u2013": "-",
    "\u2014": "-", "\u2015": "-", "\u2212": "-",
    "\u2026": "...", "\u00a0": " ", "\u202f": " ", "\u2007": " ",
    "\u2009": " ", "\u200b": "", "\u2060": "", "\u00ad": "",
    "\ufeff": "",
})
def normalize_display_text(text):
    return re.sub(r"\s+", " ", text.translate(DISPLAY_PUNCTUATION)).strip()

REF = re.compile(r"^\s*(.*?)\s+(\d{1,3}):(\d{1,3})(?:-\d+)?\s+(.+?)\s*$")
def parse_line(line):
    m = REF.match(line.replace("\ufeff", ""))
    if not m:
        return None
    book = ALIASES.get(normal(m[1]))
    if book is None:
        return None
    ch, verse = int(m[2]), int(m[3])
    text = normalize_display_text(m[4].strip())
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
            content=archive.read(item).decode("utf-8-sig", errors="strict")
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
    if not unique.get((0,1,1)) or not unique.get((65,22,21)):
        raise ValueError("Expected first and last passages missing")
    return "".join(f"{b}\t{c}\t{v}\t{t.replace(chr(9),' ')}\n" for (b,c,v),t in sorted(unique.items()))
def main():
    p=argparse.ArgumentParser()
    p.add_argument("output",type=pathlib.Path)
    p.add_argument("--input",type=pathlib.Path,help="Local ZIP (for deterministic offline test/rebuild)")
    p.add_argument("--translation",choices=sorted(TRANSLATIONS),default="en")
    args=p.parse_args()
    if args.input:
        data=args.input.read_bytes()
    else:
        data = None
        for url in sources(args.translation):
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
    translation_id, title = TRANSLATIONS[args.translation]
    args.output.write_text(f"# {title} - {translation_id}, public domain, https://ebible.org/details.php?id={translation_id}\n"+result,encoding="utf-8")
    print(f"Bundled {result.count(chr(10))} offline verses; SHA256:",hashlib.sha256(args.output.read_bytes()).hexdigest())
if __name__=="__main__":
    main()
