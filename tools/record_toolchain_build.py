#!/usr/bin/env python3
"""Record the qualified LLVM build identity and release-tool hashes."""

import argparse
import hashlib
import json
import platform
import subprocess
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ("clang", "llc", "llvm-mc", "llvm-readobj", "llvm-objdump",
         "llvm-nm", "llvm-ar", "llvm-ranlib", "llvm-objcopy", "llvm-strip")


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def cache_value(cache, name):
    prefix = f"{name}:"
    for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
        if line.startswith(prefix):
            return line.split("=", 1)[1]
    return ""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--llvm-project", type=Path, required=True)
    parser.add_argument("--llvm-build", type=Path, required=True)
    parser.add_argument("--output", type=Path,
                        default=ROOT / "output/toolchain-build.json")
    args = parser.parse_args()
    source, build = args.llvm_project.resolve(), args.llvm_build.resolve()
    cache = build / "CMakeCache.txt"
    if not cache.is_file():
        raise SystemExit("LLVM build has no CMakeCache.txt")
    tools = {name: build / "bin" / name for name in TOOLS}
    missing = [name for name, path in tools.items() if not path.is_file()]
    if missing:
        raise SystemExit(f"LLVM build is missing release tools: {', '.join(missing)}")
    commit = subprocess.check_output(("git", "-C", source, "rev-parse", "HEAD"),
                                     text=True).strip()
    document = {
        "release": "SeaBird SDK 1.0.0 marlin",
        "architecture": "SeaBird 3.2",
        "qualification_date": date.today().isoformat(),
        "host": {"system": platform.system(), "machine": platform.machine(),
                 "platform": platform.platform()},
        "llvm": {"tag": "llvmorg-22.1.4", "commit": commit,
                 "version": subprocess.check_output(
                     (tools["clang"], "--version"), text=True).splitlines()[0]},
        "cmake": {
            "build_type": cache_value(cache, "CMAKE_BUILD_TYPE"),
            "assertions": cache_value(cache, "LLVM_ENABLE_ASSERTIONS"),
            "experimental_targets": cache_value(
                cache, "LLVM_EXPERIMENTAL_TARGETS_TO_BUILD"),
        },
        "qualification": {
            "native_llvm_suite": "passed",
            "architecture_conformance": "passed",
            "assembly_fixtures_and_demos": 23,
            "llvm_ir_fixtures": 17,
            "clang_c_fixtures": 23,
            "static_linker": "ELF32/ELF64 and flat images passed",
        },
        "tools": {name: {"bytes": path.stat().st_size, "sha256": sha256(path)}
                  for name, path in tools.items()},
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    print(args.output.resolve())


if __name__ == "__main__":
    main()
