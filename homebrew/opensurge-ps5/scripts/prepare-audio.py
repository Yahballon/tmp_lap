#!/usr/bin/env python3
"""Stage exact original Open Surge audio for the native PS5 48k stereo mixer.

No music/sfx substitutions: read pinned Open Surge source checkout, convert only
sample format via ffmpeg. The complete original game files are staged separately.
"""
from __future__ import annotations
import json
import subprocess
import sys
import wave
from pathlib import Path

SFX = (
    "jump", "collectible", "collectible_loss", "spring", "roll",
    "charge", "charging_up", "release", "brake", "damaged", "death",
    "destroy", "spikes", "shot", "endsign", "checkpoint",
)
STAGE_TRACKS = {
    "demo-1": "musics/template.ogg",
    "sunshine-1": "musics/sunshine.ogg",
    "sunshine-2": "musics/sunshine.ogg",
    "sunshine-3": "musics/sunshine.ogg",
}

def ffmpeg(args: list[str]) -> None:
    subprocess.run(["ffmpeg", "-hide_banner", "-nostdin", "-loglevel", "error",
                    "-y", *args], check=True, timeout=180)

def prepare(staged: Path, app: Path) -> None:
    game=staged / "game"
    if not (game / "levels/demo-1.lev").exists():
        raise ValueError("Original level missing")
    for stage,track in STAGE_TRACKS.items():
        level=(game/"levels"/(stage+".lev")).read_text(encoding="utf-8")
        if f'music "{track}"' not in level:
            raise ValueError(f"{stage}: pinned upstream track changed")

    dest=app / "assets" / "audio-cache"
    dest.mkdir(parents=True, exist_ok=True)
    for name in SFX:
        source=game / "samples" / (name+".wav")
        if not source.is_file():raise FileNotFoundError(source)
        target=dest / (name+".wav")
        ffmpeg(["-i",str(source),"-vn","-ac","2","-ar","48000",
                "-c:a","pcm_s16le",str(target)])
        with wave.open(str(target),"rb") as wav:
            if (wav.getnchannels(),wav.getframerate(),wav.getsampwidth()) != (2,48000,2):
                raise AssertionError(f"{name} invalid audio format")
            if wav.getnframes()<100 or wav.getnframes()>48_000*15:
                raise AssertionError(f"{name} unexpected sound length")
    for name,output in (("template","level.ogg"),("sunshine","sunshine.ogg")):
        source=game/"musics"/(name+".ogg")
        if not source.is_file():raise FileNotFoundError(source)
        target=dest/output
        ffmpeg(["-i",str(source),"-vn","-ac","2","-ar","48000",
                "-c:a","libvorbis","-q:a","4",str(target)])
        probe=json.loads(subprocess.check_output(
            ["ffprobe","-v","error","-show_streams","-of","json",str(target)],
            timeout=20))
        streams=[x for x in probe["streams"] if x.get("codec_type")=="audio"]
        if len(streams)!=1 or streams[0].get("codec_name")!="vorbis" or (
            streams[0].get("sample_rate")!="48000" or streams[0].get("channels")!=2
        ): raise ValueError(f"Invalid {name} PS5 Vorbis")
        if not 10000<target.stat().st_size<32*1024*1024:
            raise ValueError(f"Unexpected {name} Vorbis length")
        print("ORIGINAL_STAGE_MUSIC",name,output,target.stat().st_size)
    manifest={
        "source":"Open Surge upstream pinned original sound assets",
        "music":"musics/template.ogg",
        "music_output":"audio-cache/level.ogg",
        "stage_music":STAGE_TRACKS,
        "stage_outputs":{"demo-1":"audio-cache/level.ogg",
                         "sunshine-1":"audio-cache/sunshine.ogg",
                         "sunshine-2":"audio-cache/sunshine.ogg",
                         "sunshine-3":"audio-cache/sunshine.ogg"},
        "sfx":list(SFX),
        "sample_rate":48000,"channels":2,
    }
    (dest/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
    print("ORIGINAL_AUDIO_STAGED",len(SFX),"sound effects",
          "2 authentic stage music tracks")

if __name__=="__main__":
    if len(sys.argv)!=3:raise SystemExit("Usage: prepare-audio.py <staged> <app>")
    prepare(Path(sys.argv[1]),Path(sys.argv[2]))
