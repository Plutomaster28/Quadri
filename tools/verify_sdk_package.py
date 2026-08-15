#!/usr/bin/env python3
"""Verify a packaged SeaBird SDK archive, manifest, and target registration."""

import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

TOOLS = ("clang", "llc", "llvm-mc", "llvm-objdump", "llvm-readobj",
         "llvm-nm", "llvm-ar", "llvm-ranlib", "llvm-objcopy", "llvm-strip",
         "seabird-ref", "pebble-xlate")


def sha256(path: Path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("archive", type=Path)
    args = parser.parse_args()
    archive = args.archive.resolve()
    checksum = archive.with_suffix(archive.suffix + ".sha256")
    if not archive.is_file() or not checksum.is_file():
        raise SystemExit("SDK archive or companion SHA256 file is missing")
    expected = checksum.read_text(encoding="utf-8").split()[0]
    if sha256(archive) != expected:
        raise SystemExit("SDK archive SHA256 mismatch")

    with tempfile.TemporaryDirectory(prefix="verify-seabird-sdk-") as temporary:
        root = Path(temporary)
        with zipfile.ZipFile(archive) as package:
            bad = package.testzip()
            if bad:
                raise SystemExit(f"corrupt ZIP member: {bad}")
            package.extractall(root)
        entries = list(root.iterdir())
        if len(entries) != 1 or not entries[0].is_dir():
            raise SystemExit("SDK archive must contain one package root")
        sdk = entries[0]
        manifest = sdk / "MANIFEST.sha256"
        if not manifest.is_file():
            raise SystemExit("SDK manifest is missing")
        recorded = {}
        for line in manifest.read_text(encoding="utf-8").splitlines():
            digest, relative = line.split("  ", 1)
            recorded[relative] = digest
        files = sorted(path for path in sdk.rglob("*") if path.is_file() and
                       path != manifest)
        if set(recorded) != {path.relative_to(sdk).as_posix() for path in files}:
            raise SystemExit("SDK manifest file list mismatch")
        for path in files:
            relative = path.relative_to(sdk).as_posix()
            if sha256(path) != recorded[relative]:
                raise SystemExit(f"SDK manifest hash mismatch: {relative}")
        for name in TOOLS:
            tool = sdk / "bin" / name
            if not tool.is_file():
                raise SystemExit(f"packaged tool is missing: {name}")
            tool.chmod(tool.stat().st_mode | 0o755)
        version = subprocess.check_output((sdk / "bin/llvm-mc", "--version"),
                                          text=True)
        if "seabird32 - SeaBird 32-bit" not in version or "seabird64 - SeaBird 64-bit" not in version:
            raise SystemExit("packaged llvm-mc has no SeaBird targets")
        macros = subprocess.check_output(
            (sdk / "bin/clang", "-target", "seabird32-unknown-none",
             "-mcpu=axium-m-v1", "-dM", "-E", "-x", "c", "/dev/null"),
            text=True)
        for macro in ("__SEABIRD_AXIUM_M__", "__SEABIRD_PAE32__",
                      "__SEABIRD_REGISTER_WINDOWS__"):
            if macro not in macros:
                raise SystemExit(f"packaged Clang is missing {macro}")
        smoke = sdk / "examples/c-smoke.c"
        object64 = root / "package-smoke64.o"
        object32 = root / "package-smoke32.o"
        common = ("-O2", "-ffreestanding", "-fno-builtin", "-c", smoke)
        subprocess.run((sdk / "bin/clang", "-target",
                        "seabird64-unknown-none", *common, "-o", object64),
                       check=True)
        subprocess.run((sdk / "bin/clang", "-target",
                        "seabird32-unknown-none", "-mcpu=axium-m-v1", *common,
                        "-o", object32), check=True)
        header64 = subprocess.check_output(
            (sdk / "bin/llvm-readobj", "--file-headers", object64), text=True)
        header32 = subprocess.check_output(
            (sdk / "bin/llvm-readobj", "--file-headers", object32), text=True)
        if "Format: elf64-seabird" not in header64:
            raise SystemExit("packaged Clang did not produce SeaBird ELF64")
        if ("Format: elf32-seabird" not in header32 or
                "Flags [ (0x3)" not in header32):
            raise SystemExit("packaged Clang did not produce Axium PAE/window ELF32")
        subprocess.run((sdk / "bin/llvm-objdump", "-d", object64),
                       check=True, stdout=subprocess.DEVNULL)
        subprocess.run((sdk / "bin/llvm-objdump", "-d", object32),
                       check=True, stdout=subprocess.DEVNULL)
        showcases = (
            ("seabird64-showcase", "seabird64-unknown-none", "",
             "seabird64_showcase", "elf64-seabird", "0x0"),
            ("axium-pae-window-showcase", "seabird32-unknown-none",
             "axium-m-v1", "axium_pae_window_showcase", "elf32-seabird",
             "0x3"),
            ("tritium-showcase", "seabird32-unknown-none", "tritium-v1",
             "tritium_showcase", "elf32-seabird", "0x0"),
        )
        for stem, triple, cpu, entry, output_format, flags in showcases:
            source = sdk / f"examples/isa-showcase/{stem}.s"
            obj = root / f"{stem}.o"
            executable = root / f"{stem}.elf"
            cpu_arg = (f"-mcpu={cpu}",) if cpu else ()
            subprocess.run((sdk / "bin/llvm-mc", f"-triple={triple}",
                            *cpu_arg, "-filetype=obj", source, "-o", obj),
                           check=True)
            subprocess.run((sys.executable, sdk / "bin/link_seabird.py",
                            "--format", "elf", "--entry", entry, "-o",
                            executable, obj), check=True,
                           stdout=subprocess.DEVNULL)
            headers = subprocess.check_output(
                (sdk / "bin/llvm-readobj", "--file-headers", executable),
                text=True)
            if (f"Format: {output_format}" not in headers or
                    f"Flags [ ({flags})" not in headers):
                raise SystemExit(f"packaged showcase link failed: {stem}")
        subprocess.run((sdk / "bin/seabird-ref", "--self-test"), check=True)
        if (sdk / "VERSION").read_text().strip() != "1.0.0":
            raise SystemExit("packaged VERSION is not 1.0.0")
        build_record = json.loads((sdk / "TOOLCHAIN_BUILD.json").read_text())
        if (build_record.get("llvm", {}).get("tag") != "llvmorg-22.1.4" or
                build_record.get("cmake", {}).get("assertions") != "ON"):
            raise SystemExit("packaged toolchain build identity is not qualified")
        for name, identity in build_record.get("tools", {}).items():
            if sha256(sdk / "bin" / name) != identity.get("sha256"):
                raise SystemExit(f"packaged tool differs from build record: {name}")
        required_docs = tuple(sdk.glob("docs/pdf/volume-*.pdf"))
        if len(required_docs) != 5 or any(not path.stat().st_size for path in required_docs):
            raise SystemExit("packaged release manual set is incomplete")
        if len(tuple(sdk.glob("conformance/*-vectors.json"))) != 3:
            raise SystemExit("packaged conformance vector set is incomplete")
    print(f"SDK package verified: {archive.name} ({archive.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
