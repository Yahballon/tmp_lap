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
    def test_available_translations(self):
        self.assertEqual(set(bible.TRANSLATIONS), {"en","de","fr","es","ru"})
        self.assertTrue(bible.sources("de")[0].endswith("/deu1912_vpl.zip"))
        self.assertTrue(bible.sources("fr")[0].endswith("/fraLSG_vpl.zip"))
        self.assertTrue(bible.sources("es")[0].endswith("/spaRV1909_vpl.zip"))
        self.assertTrue(bible.sources("ru")[0].endswith("/russyn_vpl.zip"))
    def test_refs(self):
        self.assertEqual(bible.parse_line("Gen 1:1 In the beginning"),(0,1,1,"In the beginning"))
        self.assertEqual(bible.parse_line("1Co 13:4 Love is patient"),(45,13,4,"Love is patient"))
        self.assertEqual(bible.parse_line("Revelation 22:21 Amen."),(65,22,21,"Amen."))
        self.assertEqual(bible.parse_line("Бытие 1:1 В начале сотворил Бог небо и землю."),(0,1,1,"В начале сотворил Бог небо и землю."))
        self.assertEqual(bible.parse_line("Мф 1:1 Родословие Иисуса Христа."),(39,1,1,"Родословие Иисуса Христа."))
        self.assertIsNone(bible.parse_line("random heading"))
        self.assertIsNone(bible.parse_line("Gen 1000:1 Invalid"))
    def test_typographic_punctuation_from_webpb(self):
        verse = bible.parse_line("Gen 1:2 The earth was empty. Darkness was on the surface, and God’s Spirit moved.")
        self.assertEqual(verse[3], "The earth was empty. Darkness was on the surface, and God's Spirit moved.")
        verse = bible.parse_line("Gen 1:3 God said, “Let there be light,” and there was light.")
        self.assertEqual(verse[3], 'God said, "Let there be light," and there was light.')
        verse = bible.parse_line("Gen 1:5 God called the light ‘day’, and the darkness ‘night’.")
        self.assertEqual(verse[3], "God called the light 'day', and the darkness 'night'.")
        self.assertEqual(bible.normalize_display_text("Grace—and truth… Glory\u00a0to God."), "Grace-and truth... Glory to God.")
        self.assertEqual(bible.normalize_display_text("‘test’ “quote”"), """'test' "quote" """.strip())
    def test_invalid_utf8_is_not_silently_replaced(self):
        with self.assertRaises(UnicodeDecodeError):
            bytes([0xff]).decode("utf-8-sig", errors="strict")
    def test_rejects_partial_corpus(self):
        stream=io.BytesIO()
        with zipfile.ZipFile(stream,"w") as z:
            z.writestr("verses.txt","Gen 1:1 In the beginning\nRev 22:21 Amen.\n")
        with self.assertRaises(ValueError):
            bible.convert_zip(stream.getvalue())
if __name__=="__main__":
    unittest.main()
