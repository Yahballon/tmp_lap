import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("patch_ps5", PROJECT / "scripts" / "patch-ps5.py")
patch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)


class PortIntegrationTests(unittest.TestCase):
    def test_pinned_sources(self):
        lock = json.loads((PROJECT / "upstream.lock.json").read_text())
        self.assertEqual(lock["version"], 1)
        for name in ("opensurge", "surgescript", "allegro", "physfs"):
            self.assertRegex(lock[name]["commit"], r"^[0-9a-f]{40}$")
            self.assertNotIn("master", lock[name]["tag"])

    def test_native_c_library_cross_compiler_is_not_a_fake_game(self):
        native = (PROJECT / "scripts" / "build-ps5-libs.sh").read_text()
        self.assertIn("surgescript-static", native)
        self.assertIn("physfs-static", native)
        self.assertNotIn("gh release create", native)
        self.assertNotIn("eboot.bin", native)
        toolchain = (PROJECT / "ps5" / "ps5-static-toolchain.cmake").read_text()
        self.assertIn("x86_64-sie-ps5", toolchain)
        self.assertIn("CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY", toolchain)

    def test_ps5_surge_allocation_patch_keeps_non_ps5_code(self):
        spec = importlib.util.spec_from_file_location("ss5", PROJECT / "scripts" / "patch-surgescript-ps5.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            path = root / "src" / "surgescript" / "runtime" / "program.c"
            path.parent.mkdir(parents=True)
            path.write_text("prefix\\n" + module.ANCHOR + "suffix\\n")
            module.apply(root)
            patched = path.read_text()
            self.assertIn("#define alloca __builtin_alloca", patched)
            self.assertIn("#include <malloc.h>", patched)
            self.assertIn("#elif !(defined(__APPLE__)", patched)
            with self.assertRaises(ValueError):
                module.apply(root)

    def test_title_identity(self):
        meta = json.loads((PROJECT / "app.json").read_text())
        self.assertEqual(meta["titleId"], "PPSA99358")
        self.assertFalse(meta["engineIntegrated"])
        self.assertFalse(meta["installable"])
        self.assertEqual(meta["slug"], "opensurge-ps5")

    def test_patches_real_upstream_main_without_replacing_gameplay(self):
        source = '\n'.join([
            '#include "core/commandline.h"',
            'int main(int argc, char **argv) {',
            '    commandline_t cmd = commandline_parse(argc, argv);',
            '    engine_init(&cmd);',
            '    engine_mainloop();',
            '    engine_release();',
            '    return 0;',
            '}',
            ''])
        with tempfile.TemporaryDirectory() as folder:
            upstream = Path(folder)
            (upstream / "src").mkdir()
            f = upstream / "src" / "main.c"
            f.write_text(source)
            patch.apply(upstream, PROJECT / "ps5" / "ps5_launcher.h")
            result = f.read_text()
            self.assertIn("opensurge_ps5_prepare_arguments(&argc, &argv)", result)
            self.assertIn("engine_mainloop();", result)
            self.assertIn("engine_release();", result)
            self.assertTrue((upstream / "src" / "ps5_launcher.h").is_file())
            with self.assertRaises(ValueError):
                patch.apply(upstream, PROJECT / "ps5" / "ps5_launcher.h")

    def test_upstream_anchor_change_fails_closed(self):
        with tempfile.TemporaryDirectory() as folder:
            upstream = Path(folder)
            (upstream / "src").mkdir()
            (upstream / "src" / "main.c").write_text("int main() {}")
            with self.assertRaises(ValueError):
                patch.apply(upstream, PROJECT / "ps5" / "ps5_launcher.h")
            self.assertFalse((upstream / "src" / "ps5_launcher.h").exists())


if __name__ == "__main__":
    unittest.main()
