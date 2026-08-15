#!/usr/bin/env python3
"""Configure and build the SeaBird LLVM/Clang 22 toolchain on Linux."""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGETS = (
    "clang", "llc", "llvm-mc", "llvm-tblgen", "llvm-readobj",
    "llvm-objdump", "llvm-nm", "llvm-ar", "llvm-ranlib", "llvm-objcopy",
    "llvm-strip",
)


def required(name: str) -> str:
    value = shutil.which(name)
    if not value:
        raise SystemExit(f"missing required host tool: {name}")
    return value


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--llvm-project", type=Path, required=True)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=min(8, os.cpu_count() or 1))
    parser.add_argument("--build-type", default="Release")
    parser.add_argument("--no-assertions", action="store_true")
    args = parser.parse_args()
    if args.jobs < 1:
        raise SystemExit("--jobs must be positive")

    subprocess.run((sys.executable, str(ROOT / "tools/install_llvm_backend.py"),
                    str(args.llvm_project)), check=True, cwd=ROOT)
    cmake, clang, clangxx = required("cmake"), required("clang"), required("clang++")
    configure = (
        cmake, "-S", str(args.llvm_project.resolve() / "llvm"),
        "-B", str(args.build_root.resolve()), "-G", "Ninja",
        f"-DCMAKE_BUILD_TYPE={args.build_type}",
        f"-DCMAKE_C_COMPILER={clang}", f"-DCMAKE_CXX_COMPILER={clangxx}",
        f"-DLLVM_ENABLE_ASSERTIONS={'OFF' if args.no_assertions else 'ON'}",
        "-DLLVM_TARGETS_TO_BUILD=", "-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=SeaBird",
        "-DLLVM_ENABLE_PROJECTS=clang", "-DLLVM_INCLUDE_TESTS=OFF",
        "-DLLVM_INCLUDE_BENCHMARKS=OFF", "-DLLVM_INCLUDE_EXAMPLES=OFF",
        "-DLLVM_ENABLE_ZLIB=OFF", "-DLLVM_ENABLE_ZSTD=OFF",
        "-DLLVM_ENABLE_LIBXML2=OFF", "-DLLVM_ENABLE_TERMINFO=OFF",
    )
    subprocess.run(configure, check=True, cwd=ROOT)
    subprocess.run((cmake, "--build", str(args.build_root.resolve()), "--target",
                    *TARGETS, "--parallel", str(args.jobs)), check=True, cwd=ROOT)
    print(f"built SeaBird LLVM tools in {args.build_root.resolve() / 'bin'}")


if __name__ == "__main__":
    main()
