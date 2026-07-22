#!/usr/bin/env python3
"""Build a deterministic SeaBird SDK v0.1 zip from a qualified LLVM build."""

import argparse
import hashlib
import os
import platform
import shutil
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = (
    "clang", "llc", "llvm-mc", "llvm-objdump", "llvm-readobj", "llvm-nm",
    "llvm-ar", "llvm-ranlib", "llvm-objcopy", "llvm-strip",
)
FIXED_ZIP_TIME = (2026, 7, 21, 0, 0, 0)


def version():
    value = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    if not value or any(ch not in "0123456789." for ch in value):
        raise SystemExit("VERSION is not a numeric dotted release version")
    return value


def host_tag():
    system = platform.system().lower()
    machine = platform.machine().lower()
    machine = {"amd64": "x86_64", "x86_64": "x86_64",
               "aarch64": "aarch64", "arm64": "aarch64"}.get(machine, machine)
    return f"{system}-{machine}"


def find_tool(bin_dir, name):
    candidates = (bin_dir / name, bin_dir / f"{name}.exe")
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise SystemExit(f"missing required SDK tool: {name} in {bin_dir}")


def copy_file(source, destination, executable=False):
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination, follow_symlinks=True)
    if executable:
        destination.chmod(destination.stat().st_mode | 0o755)


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def write_manifest(stage):
    files = sorted(path for path in stage.rglob("*") if path.is_file() and
                   path.name != "MANIFEST.sha256")
    text = "".join(
        f"{sha256(path)}  {path.relative_to(stage).as_posix()}\n"
        for path in files
    )
    (stage / "MANIFEST.sha256").write_text(text, encoding="utf-8")


def write_zip(stage, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path in sorted(p for p in stage.rglob("*") if p.is_file()):
            relative = path.relative_to(stage.parent).as_posix()
            info = zipfile.ZipInfo(relative, FIXED_ZIP_TIME)
            mode = 0o755 if os.access(path, os.X_OK) else 0o644
            info.external_attr = (mode & 0xFFFF) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            with path.open("rb") as stream:
                archive.writestr(info, stream.read(), compresslevel=9)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--llvm-build", type=Path, required=True,
                        help="qualified LLVM build directory")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "dist")
    parser.add_argument("--platform", default=host_tag(), dest="platform_tag")
    args = parser.parse_args()

    release = version()
    package_name = f"seabird-sdk-v{release}-tuna-{args.platform_tag}"
    llvm_bin = args.llvm_build.resolve() / "bin"
    resource_root = args.llvm_build.resolve() / "lib" / "clang" / "22"
    resource_headers = resource_root / "include"
    if not resource_headers.is_dir():
        raise SystemExit(f"missing Clang resource headers: {resource_headers}")

    with tempfile.TemporaryDirectory(prefix="seabird-sdk-") as temporary:
        stage = Path(temporary) / package_name
        stage.mkdir()
        for tool in TOOLS:
            source = find_tool(llvm_bin, tool)
            copy_file(source, stage / "bin" / source.name, executable=True)
        copy_file(ROOT / "tools/link_seabird.py", stage / "bin/link_seabird.py",
                  executable=True)
        shutil.copytree(resource_headers, stage / "lib/clang/22/include")
        shutil.copytree(ROOT / "runtime", stage / "runtime")
        shutil.copytree(ROOT / "spec", stage / "spec")
        for source, target in (
            (ROOT / "VERSION", stage / "VERSION"),
            (ROOT / "RELEASE", stage / "RELEASE"),
            (ROOT / "LICENSE", stage / "LICENSE"),
            (ROOT / "CHANGELOG.md", stage / "CHANGELOG.md"),
            (ROOT / "packaging/SDK_README.md", stage / "README.md"),
            (ROOT / "docs/releases/v0.1.0.md", stage / "RELEASE_NOTES.md"),
            (ROOT / "docs/releases/v0.1.0-validation.md", stage / "VALIDATION.md"),
            (ROOT / "docs/COMPILER_STATUS.md", stage / "COMPILER_STATUS.md"),
            (ROOT / "docs/ARCHITECTURE_STATUS.md", stage / "ARCHITECTURE_STATUS.md"),
            (ROOT / "tests/llvm/c-smoke.c", stage / "examples/c-smoke.c"),
            (ROOT / "tests/llvm/c-native-fp128.c", stage / "examples/c-native-fp128.c"),
            (ROOT / "examples/seabird64/arithmetic.s", stage / "examples/seabird64/arithmetic.s"),
            (ROOT / "examples/seabird64/floating.s", stage / "examples/seabird64/floating.s"),
            (ROOT / "examples/tritium/basic.s", stage / "examples/tritium/basic.s"),
        ):
            copy_file(source, target)
        write_manifest(stage)
        output = args.output_dir.resolve() / f"{package_name}.zip"
        write_zip(stage, output)
        checksum = output.with_suffix(output.suffix + ".sha256")
        checksum.write_text(f"{sha256(output)}  {output.name}\n",
                            encoding="utf-8")
    print(output)


if __name__ == "__main__":
    main()
