import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("stage_data", PROJECT / "scripts" / "stage-game-data.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class StageGameDataTests(unittest.TestCase):
    def _fixture(self, base: Path) -> Path:
        src = base / "opensurge"
        src.mkdir()
        for name in module.ROOT_FILES:
            (src / name).write_bytes(("upstream " + name).encode())
        for name in module.ASSET_DIRS:
            (src / name).mkdir()
            (src / name / "sample.txt").write_text("original asset", encoding="utf-8")
        return src

    def test_full_manifest_and_upstream_files(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            src = self._fixture(root)
            result = module.stage(src, root / "out")
            self.assertEqual(result["title_id"], "PPSA99358")
            self.assertEqual(result["file_count"], len(module.ROOT_FILES) + len(module.ASSET_DIRS))
            self.assertEqual(
                json.loads((root / "out" / "game-data-manifest.json").read_text()),
                result,
            )
            self.assertEqual(
                (root / "out" / "game" / "surge.cfg").read_bytes(),
                (src / "surge.cfg").read_bytes(),
            )
            with self.assertRaises(ValueError):
                module.stage(src, root / "out")

    def test_refuses_missing_and_outside_repository_asset(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            src = self._fixture(root)
            (src / "surge.rocks").unlink()
            with self.assertRaises(FileNotFoundError):
                module.stage(src, root / "missing")
            (src / "surge.rocks").symlink_to(root / "external")
            (root / "external").write_text("do not package")
            with self.assertRaises(ValueError):
                module.stage(src, root / "unsafe")


if __name__ == "__main__":
    unittest.main()
