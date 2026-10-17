import importlib.util
import io
import pathlib
import sys
import unittest
import zipfile

MODULE = pathlib.Path(__file__).resolve().parents[1]/"tools"/"prepare_bible.py"
spec = importlib.util.spec_from_file_location("prepare_bible", MODULE)
bible = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bible)

class BibleConverterTests(unittest.TestCase):
    def test_refs(self):
        self.assertEqual(bible.parse_line("Gen 1:1 In the beginning"),(0,1,1,"In the beginning"))
        self.assertEqual(bible.parse_line("1Co 13:4 Love is patient"),(45,13,4,"Love is patient"))
        self.assertEqual(bible.parse_line("Revelation 22:21 Amen."),(65,22,21,"Amen."))
        self.assertIsNone(bible.parse_line("random heading"))
        self.assertIsNone(bible.parse_line("Gen 1000:1 Invalid"))
    def test_rejects_partial_corpus(self):
        stream=io.BytesIO()
        with zipfile.ZipFile(stream,"w") as z:
            z.writestr("verses.txt","Gen 1:1 In the beginning\nRev 22:21 Amen.\n")
        with self.assertRaises(ValueError):
            bible.convert_zip(stream.getvalue())
if __name__=="__main__":
    unittest.main()
