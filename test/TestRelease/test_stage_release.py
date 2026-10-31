import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile

spec = importlib.util.spec_from_file_location(
    "stage_release", Path(__file__).resolve().parents[2] / "tools/stage-release.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
SOURCE = "25df88ccd8e896ba4805fe31c848f40ba3d0e249"


class ReleaseAssetsTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.artifact = self.root / "artifact"
        self.artifact.mkdir()
        self.installer = self.artifact / "weasel-0.17.4.106.25df88c-installer.exe"
        self.installer.write_bytes(b"MZfixture")
        with zipfile.ZipFile(self.artifact / "p2c-rime-config.zip", "w") as archive:
            for name in ["lua/contextual_pair.lua", "rime_ice.custom.yaml", "weasel.custom.example.yaml"]:
                archive.writestr(name, "fixture")
        self.destination = self.root / "release"

    def test_valid_assets_and_checksums(self):
        module.stage(self.artifact, self.destination, SOURCE)
        self.assertEqual(len(list(self.destination.iterdir())), 3)
        for line in (self.destination / "SHA256SUMS.txt").read_text().splitlines():
            digest, name = line.split("  ")
            self.assertEqual(digest, hashlib.sha256((self.destination / name).read_bytes()).hexdigest())

    def test_wrong_build(self):
        with self.assertRaises(ValueError):
            module.stage(self.artifact, self.destination, "0" * 40)

    def test_model_weights_are_rejected(self):
        with zipfile.ZipFile(self.artifact / "p2c-rime-config.zip", "a") as archive:
            archive.writestr("model.qjm", b"weights")
        with self.assertRaises(ValueError):
            module.stage(self.artifact, self.destination, SOURCE)
        self.assertFalse(self.destination.exists())

    def test_missing_configuration(self):
        with zipfile.ZipFile(self.artifact / "p2c-rime-config.zip", "w") as archive:
            archive.writestr("README.md", "incomplete")
        with self.assertRaises(ValueError):
            module.stage(self.artifact, self.destination, SOURCE)


if __name__ == "__main__":
    unittest.main()
