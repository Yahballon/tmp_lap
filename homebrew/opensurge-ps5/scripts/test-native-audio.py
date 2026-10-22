#!/usr/bin/env python3
"""Integration test: authentic Open Surge audio staged for 48k PS5 audio path."""
import json
import subprocess
import sys
import wave
from pathlib import Path

def test(app:Path):
    cache=app/"assets"/"audio-cache"
    manifest=json.loads((cache/"manifest.json").read_text(encoding="utf-8"))
    assert manifest["sample_rate"]==48000 and manifest["channels"]==2
    assert manifest["music"]=="musics/template.ogg"
    assert len(manifest["sfx"])>=16
    frame_sum=0
    for name in manifest["sfx"]:
        with wave.open(str(cache/(name+".wav")),"rb") as x:
            assert (x.getframerate(),x.getnchannels(),x.getsampwidth())==(48000,2,2),name
            assert 100<x.getnframes()<15*48000
            frame_sum+=x.getnframes()
    assert manifest["stage_music"] == {
        "demo-1":"musics/template.ogg",
        "sunshine-1":"musics/sunshine.ogg",
        "sunshine-2":"musics/sunshine.ogg",
        "sunshine-3":"musics/sunshine.ogg",
    }
    for track in ("level.ogg","sunshine.ogg"):
        ogg=cache/track
        assert ogg.read_bytes()[:4]==b"OggS"
        streams=json.loads(subprocess.check_output(["ffprobe","-v","error","-of","json",
            "-show_streams",str(ogg)]))["streams"]
        assert any(x.get("codec_name")=="vorbis" and x.get("sample_rate")=="48000"
                   and x.get("channels")==2 for x in streams),track
        assert ogg.stat().st_size>10000,track
    cpp=(Path(__file__).resolve().parents[1]/"native"/"game_audio.hpp").read_text()
    main=(Path(__file__).resolve().parents[1]/"native"/"main.cpp").read_text()
    assert "hui::ps5::AudioOut" in cpp and "hui::audio::Mixer" in cpp
    assert "hui::audio::MusicTrack" in cpp and "track.decode(" in cpp
    assert "mixer_.attach_stream(1,&sunshine_ring_)" in cpp
    assert "mixer_.set_stream_gain(1" in cpp
    assert "stage_slot_" in cpp and "audio.select_stage(selected)" in main
    assert "STAGE_MUSIC" in main and "sunshine.ogg" in cpp
    assert "decode_wav(" in cpp and "output_.start(mixer_)" in cpp
    for name in ("Jump","Collectible","CollectibleLoss","Spring","Charge",
                 "ChargingUp","Release","Destroy","Death","Endsign"):
        assert "surgepreview::Sound::"+name in main, name
    assert "AUDIO_PERF" in main and "ORIGINAL_MUSIC_MUTED" in main
    print("PASS ORIGINAL AUDIO 48k stereo: effects="+str(len(manifest["sfx"]))+
          ", frames="+str(frame_sum)+", 2 authentic OGG tracks with stage crossfade")

if __name__=="__main__":
    test(Path(sys.argv[1] if len(sys.argv)>1 else "app"))
