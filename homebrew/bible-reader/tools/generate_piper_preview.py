#!/usr/bin/env python3
"""Build-time Piper neural voice preview. Produces only PCM, not an ONNX PS5 runtime.
All 31 Luther 1912 verses of Genesis 1 can be played offline on console; the
rest of the Bible keeps using the proven eSpeak NG fallback. Do NOT label as
on-device Piper: speech synthesis executes on the CI host, not on the PS5.
"""
import argparse
import hashlib
import pathlib
import struct
import wave

BOOK, CHAPTER = 0, 1
SAMPLING_RATE = 22050
MAX_SAMPLES = SAMPLING_RATE * 80

def german_genesis(path):
    verses={}
    for line in pathlib.Path(path).read_text(encoding="utf-8").splitlines():
        if line.startswith("#"):
            continue
        cells=line.split("\t",3)
        if len(cells)!=4:
            raise ValueError("Malformed verse corpus")
        book,chapter,num=map(int,cells[:3])
        if book==BOOK and chapter==CHAPTER:
            if num in verses:
                raise ValueError("Duplicate verse")
            verses[num]=cells[3]
    if len(verses)!=31 or sorted(verses)!=list(range(1,32)):
        raise ValueError("Genesis 1 is not a complete 31-verse chapter")
    return verses

def generate(model,config,corpus,dest,manifest):
    from piper import PiperVoice
    model=pathlib.Path(model)
    model_bytes=model.read_bytes()
    digest=hashlib.sha256(model_bytes).hexdigest()
    expected="7e64762d8e5118bb578f2eea6207e1a35a8e0c30595010b666f983fc87bb7819"
    if digest!=expected:
        raise ValueError("Unexpected Thorsten ONNX model SHA256: "+digest)
    if not pathlib.Path(config).is_file():
        raise ValueError("Missing voice configuration")
    voice=PiperVoice.load(str(model),config_path=str(config))
    lines=german_genesis(corpus)
    root=pathlib.Path(dest)
    root.mkdir(parents=True,exist_ok=True)
    info=["# Piper Thorsten Medium, Lutherbibel 1912, 22.05kHz, 16bit mono PCM",
          "# Synthesis performed on Linux CI host, not on PS5. Preview is Genesis 1 only.",
          "format=pcm_s16le","sample_rate=22050","book=0","chapter=1","verse_count=31"]
    total_bytes=0
    for verse,text in sorted(lines.items()):
        target=root/f"{verse}.pcm"
        wavpath=root/f"{verse}.wav"
        with wave.open(str(wavpath),"wb") as wav:
            voice.synthesize_wav(text,wav)
        with wave.open(str(wavpath),"rb") as wav:
            if wav.getframerate()!=SAMPLING_RATE or wav.getnchannels()!=1 or wav.getsampwidth()!=2:
                raise ValueError(f"Unexpected audio format in verse {verse}")
            frames=wav.getnframes()
            if not (100<frames<MAX_SAMPLES):
                raise ValueError(f"Verse {verse} duration invalid: {frames}")
            audio=wav.readframes(frames)
            if len(audio)!=frames*2:
                raise ValueError("Truncated synthesis")
            target.write_bytes(audio)
        wavpath.unlink()
        info.append(f"verse={verse} samples={frames} sha256={hashlib.sha256(audio).hexdigest()}")
        total_bytes+=len(audio)
        print(f"Neural Genesis 1:{verse}: {frames/SAMPLING_RATE:.2f}s")
    pathlib.Path(manifest).write_text("\n".join(info)+"\n",encoding="utf-8")
    print(f"German Piper preview complete: 31 verses, {total_bytes:,} bytes PCM")
    return total_bytes

def verify_folder(source, manifest):
    rows=pathlib.Path(manifest).read_text(encoding="utf-8").splitlines()
    if not any(row=="sample_rate=22050" for row in rows):
        raise ValueError("Missing audio format metadata")
    if not any(row=="verse_count=31" for row in rows):
        raise ValueError("Missing verse count")
    total=0
    for verse in range(1,32):
        data=(pathlib.Path(source)/f"{verse}.pcm").read_bytes()
        if len(data)<200 or len(data)%2:
            raise ValueError("Bad PCM audio for verse "+str(verse))
        digest=hashlib.sha256(data).hexdigest()
        if not any(f"verse={verse} " in line and f"sha256={digest}" in line for line in rows):
            raise ValueError(f"Checksum mismatch for verse {verse}")
        total+=len(data)
    print("Verified 31 offline Piper PCM verses:",total,"bytes")
    return total

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--model",type=pathlib.Path)
    parser.add_argument("--config",type=pathlib.Path)
    parser.add_argument("--corpus",type=pathlib.Path)
    parser.add_argument("--output",required=True,type=pathlib.Path)
    parser.add_argument("--verify-only",action="store_true")
    args=parser.parse_args()
    manifest=args.output/"preview-manifest.txt"
    if not args.verify_only:
        for path in (args.model,args.config,args.corpus):
            if path is None:
                parser.error("Model, config and corpus are required to generate")
        generate(args.model,args.config,args.corpus,args.output,manifest)
    verify_folder(args.output,manifest)

if __name__=="__main__":
    main()
