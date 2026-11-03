#!/usr/bin/env python3
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from fix_dynarmic_log import DECL, MARKER, RELATIVE, patch

class TestDynarmicLogDeclaration(unittest.TestCase):
    def test_declares_c_abi_once_and_preserves_jit_code(self):
        with tempfile.TemporaryDirectory() as base:
            path = Path(base) / RELATIVE
            path.parent.mkdir(parents=True)
            original = '#include <cstdint>\n' + MARKER + '\nvoid test() { droid_log("hello"); }\n}\n'
            path.write_text(original)
            self.assertTrue(patch(Path(base)))
            changed = path.read_text()
            self.assertIn(DECL + '\n\n' + MARKER, changed)
            self.assertIn('void test() { droid_log("hello"); }', changed)
            self.assertFalse(patch(Path(base)))
            self.assertEqual(changed, path.read_text())
    def test_fails_closed_on_upstream_structure_change(self):
        with tempfile.TemporaryDirectory() as base:
            path = Path(base) / RELATIVE
            path.parent.mkdir(parents=True)
            path.write_text("namespace Other {}")
            with self.assertRaisesRegex(RuntimeError, "anchor changed"):
                patch(Path(base))

if __name__ == "__main__":
    unittest.main()
