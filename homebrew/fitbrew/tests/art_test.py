"""FITBREW generated image acceptance: dimensions, nonempty opacity, exact GPU raw bytes."""
from pathlib import Path
from PIL import Image
import subprocess
import tempfile
import hashlib

source=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as d:
    stage=Path(d)
    subprocess.run(["python3",str(source/"assets"/"generate_assets.py"),str(stage)],check=True)
    assert Image.open(stage/"sce_sys"/"icon0.png").size == (512,512)
    distinct=set()
    for i in range(6):
        jpg=stage/"assets"/f"pose_{i}.jpg"
        raw=stage/"assets"/f"pose_{i}.rgba"
        assert jpg.stat().st_size > 4096
        assert raw.stat().st_size == 512*512*4
        with Image.open(jpg) as img:
            assert img.size == (512,512)
            distinct.add(hashlib.sha256(jpg.read_bytes()).hexdigest())
        data=raw.read_bytes()
        assert any(v != 0 for v in data[:2048])
        assert all(data[p]==255 for p in range(3,len(data),4)),f"alpha not opaque: {i}"
    assert len(distinct)==6,"Images are unexpectedly duplicated"
print("PASS FITBREW art: six native GPU textures, JPEG fallbacks, 512x512 launcher tile")
