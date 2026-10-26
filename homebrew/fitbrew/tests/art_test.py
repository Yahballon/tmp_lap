"""FITBREW generated image acceptance: dimensions, nonempty opacity, exact GPU raw bytes."""
from pathlib import Path
from PIL import Image, ImageStat
import subprocess
import tempfile
import hashlib

source=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as d:
    stage=Path(d)
    subprocess.run(["python3",str(source/"assets"/"generate_assets.py"),str(stage)],check=True)
    assert Image.open(stage/"sce_sys"/"icon0.png").size == (512,512)
    distinct=set()
    for i in range(12):
        jpg=stage/"assets"/f"pose_{i}.jpg"
        raw=stage/"assets"/f"pose_{i}.rgba"
        assert jpg.stat().st_size > 4096
        assert raw.stat().st_size == 512*512*4
        with Image.open(jpg) as img:
            assert img.size == (512,512)
            assert ImageStat.Stat(img.convert("L")).stddev[0]>20, "Suspiciously flat artwork"
            distinct.add(hashlib.sha256(jpg.read_bytes()).hexdigest())
        data=raw.read_bytes()
        assert any(v != 0 for v in data[:2048])
        assert all(data[p]==255 for p in range(3,len(data),4)),f"alpha not opaque: {i}"
    assert len(distinct)==12,"Images are unexpectedly duplicated"
print("PASS FITBREW art: twelve native GPU textures, JPEG fallbacks, 512x512 launcher tile")
