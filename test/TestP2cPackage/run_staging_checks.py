"""Exercise build-p2c.ps1 -UseStaged without compiling or loading a DLL.

Fixtures contain minimal PE headers. These checks cover architecture, source
revision, missing files and checksums; they do not validate a working Windows DLL.
"""
from pathlib import Path
import hashlib
import json
import shutil
import struct
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
pwsh = sys.argv[1] if len(sys.argv) > 1 else "pwsh"

with tempfile.TemporaryDirectory(prefix="weasel package ") as directory:
    fixture = Path(directory)
    shutil.copy2(root / "build-p2c.ps1", fixture / "build-p2c.ps1")
    subprocess.run(["git", "init", "-q", str(fixture)], check=True)
    subprocess.run(["git", "-C", str(fixture), "-c", "user.name=Package test",
                    "-c", "user.email=package-test@example.invalid", "commit",
                    "--allow-empty", "-qm", "fixture"], check=True)
    commit = subprocess.check_output(["git", "-C", str(fixture), "rev-parse", "HEAD"], text=True).strip()
    files = [("x86_64-pc-windows-msvc", "output/weasel_p2c.dll", 0x8664),
             ("i686-pc-windows-msvc", "output/Win32/weasel_p2c.dll", 0x014c)]

    def prepare():
        entries = []
        for target, relative, machine in files:
            data = bytearray(512)
            data[:2] = b"MZ"
            struct.pack_into("<I", data, 0x3C, 0x80)
            data[0x80:0x84] = b"PE\0\0"
            struct.pack_into("<H", data, 0x84, machine)
            path = fixture / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
            entries.append({"target": target, "sha256": hashlib.sha256(data).hexdigest()})
        manifest = {"commit": commit, "binaries": entries}
        (fixture / "output/p2c-runtime.json").write_text(json.dumps(manifest))
        return manifest

    def check(success):
        result = subprocess.run([pwsh, "-NoLogo", "-NoProfile", "-ExecutionPolicy", "Bypass",
                                 "-File", str(fixture / "build-p2c.ps1"), "-UseStaged"],
                                capture_output=True, text=True)
        assert (result.returncode == 0) == success, result.stdout + result.stderr

    prepare()
    check(True)
    manifest = prepare()
    manifest["commit"] = "different-commit"
    (fixture / "output/p2c-runtime.json").write_text(json.dumps(manifest))
    check(False)
    prepare()
    (fixture / files[1][1]).unlink()
    check(False)
    manifest = prepare()
    manifest["binaries"][0]["sha256"] = "wrong-checksum"
    (fixture / "output/p2c-runtime.json").write_text(json.dumps(manifest))
    check(False)
    prepare()
    shutil.copy2(fixture / files[0][1], fixture / files[1][1])
    check(False)
    prepare()
    (fixture / files[0][1]).write_bytes(b"invalid DLL")
    check(False)
    print("P2C package staging: all checks passed")
