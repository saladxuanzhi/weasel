"""Select installer/config assets from a completed CI artifact and hash them."""
import argparse
import hashlib
from pathlib import Path, PurePosixPath
import re
import shutil
import zipfile


def stage(artifact, destination, source_sha):
    if not re.fullmatch(r"[0-9a-f]{40}", source_sha):
        raise ValueError("Expected a full source commit SHA")
    installers = list(artifact.glob("weasel-*-installer.exe"))
    if len(installers) != 1 or not installers[0].name.endswith(f".{source_sha[:7]}-installer.exe"):
        raise ValueError("Installer name does not match the validated source commit")
    with installers[0].open("rb") as stream:
        if stream.read(2) != b"MZ":
            raise ValueError("Installer is not a Windows executable")
    config = artifact / "p2c-rime-config.zip"
    with zipfile.ZipFile(config) as archive:
        names = set()
        for entry in archive.infolist():
            path = PurePosixPath(entry.filename.replace("\\", "/"))
            if path.is_absolute() or ".." in path.parts:
                raise ValueError("Unsafe configuration archive path")
            if not entry.is_dir():
                if path.suffix.lower() not in {".md", ".yaml", ".lua"}:
                    raise ValueError("Configuration archive contains an unexpected asset")
                names.add(str(path))
        if not {"lua/contextual_pair.lua", "rime_ice.custom.yaml", "weasel.custom.example.yaml"} <= names:
            raise ValueError("Configuration archive is missing required files")
        if archive.testzip() is not None:
            raise ValueError("Configuration archive failed its CRC check")
    destination.mkdir(parents=True, exist_ok=False)
    checksums = []
    for asset in [installers[0], config]:
        output = destination / asset.name
        shutil.copyfile(asset, output)
        checksums.append(f"{hashlib.sha256(output.read_bytes()).hexdigest()}  {output.name}\n")
    (destination / "SHA256SUMS.txt").write_text("".join(checksums), encoding="ascii")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifact", type=Path, required=True)
    parser.add_argument("--destination", type=Path, required=True)
    parser.add_argument("--source-sha", required=True)
    args = parser.parse_args()
    stage(args.artifact, args.destination, args.source_sha)
