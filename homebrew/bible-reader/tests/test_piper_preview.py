import importlib.util
import pathlib
import tempfile
import unittest
S=pathlib.Path(__file__).resolve().parents[1]/"tools"/"generate_piper_preview.py"
mod=importlib.util.module_from_spec(importlib.util.spec_from_file_location("preview",S))
mod.__spec__.loader.exec_module(mod)
class PreviewTests(unittest.TestCase):
    def test_validated_corpus(self):
        with tempfile.TemporaryDirectory() as root:
            path=pathlib.Path(root)/"de.tsv"
            path.write_text("# header\n"+"".join(
                f"0\t1\t{i}\tText {i}\n" for i in range(1,32)),encoding="utf-8")
            rows=mod.german_genesis(path)
            self.assertEqual(len(rows),31)
            self.assertEqual(rows[1],"Text 1")
            path.write_text("0\t1\t1\tIncomplete",encoding="utf-8")
            with self.assertRaises(ValueError):mod.german_genesis(path)
    def test_manifest_and_pcm(self):
        with tempfile.TemporaryDirectory() as root:
            root=pathlib.Path(root)
            lines=["sample_rate=22050","verse_count=31"]
            import hashlib
            for i in range(1,32):
                audio=b"\x00\x00"*200
                (root/f"{i}.pcm").write_bytes(audio)
                lines.append(f"verse={i} samples=200 sha256={hashlib.sha256(audio).hexdigest()}")
            (root/"preview-manifest.txt").write_text("\n".join(lines))
            self.assertEqual(mod.verify_folder(root,root/"preview-manifest.txt"),31*400)
            (root/"1.pcm").write_bytes(b"broken")
            with self.assertRaises(ValueError):mod.verify_folder(root,root/"preview-manifest.txt")
if __name__=="__main__":
    unittest.main()
