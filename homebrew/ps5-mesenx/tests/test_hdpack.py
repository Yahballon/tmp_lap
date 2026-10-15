import sys
from pathlib import Path
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from hdpack import install


class HdPackTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def pack(self, files):
        z = self.root / "sample.zip"
        with zipfile.ZipFile(z, "w") as archive:
            for name, contents in files.items():
                archive.writestr(name, contents)
        return z

    def test_nested_pack_root(self):
        z = self.pack({"Zelda Remastered/hires.txt": "<ver>106</ver>",
                       "Zelda Remastered/images/tiles.png": b"PNG"})
        target = install(z, "The Legend of Zelda.nes", self.root / "HdPacks")
        self.assertTrue((target / "hires.txt").exists())
        self.assertTrue((target / "images/tiles.png").exists())

    def test_traversal_rejected(self):
        z = self.pack({"hires.txt": "ok", "../outside.txt": "bad"})
        with self.assertRaises(ValueError):
            install(z, "Zelda.nes", self.root / "HdPacks")

    def test_no_hires_rejected(self):
        z = self.pack({"images/foo.png": b"x"})
        with self.assertRaises(ValueError):
            install(z, "Zelda.nes", self.root / "HdPacks")

    def test_prevent_overwrite(self):
        z = self.pack({"hires.txt": "<ver>106</ver>"})
        install(z, "Zelda.nes", self.root / "HdPacks")
        with self.assertRaises(FileExistsError):
            install(z, "Zelda.nes", self.root / "HdPacks")

    def test_invalid_rom_path(self):
        z = self.pack({"hires.txt": "ok"})
        with self.assertRaises(ValueError):
            install(z, "../../Zelda.nes", self.root / "HdPacks")


if __name__ == "__main__":
    unittest.main()
