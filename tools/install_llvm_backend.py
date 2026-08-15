#!/usr/bin/env python3
"""Install the project-owned SeaBird overlay into an LLVM 22.1.4 checkout."""

import argparse
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PATCHES = (
    ROOT / "llvm/patches/llvm-22-seabird-triple.patch",
    ROOT / "llvm/patches/llvm-22-seabird-elf.patch",
    ROOT / "llvm/patches/clang-22-seabird-target.patch",
)


def git(checkout: Path, *args: str, check: bool = True):
    return subprocess.run(
        ("git", "-c", f"safe.directory={checkout}", "-C", str(checkout), *args),
        check=check, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("llvm_project_root", type=Path)
    args = parser.parse_args()
    checkout = args.llvm_project_root.resolve()
    if not (checkout / "llvm/CMakeLists.txt").is_file():
        raise SystemExit(f"not an llvm-project source root: {checkout}")
    clang_targets = checkout / "clang/lib/Basic/Targets"
    if not clang_targets.is_dir():
        raise SystemExit(f"LLVM checkout does not contain Clang sources: {checkout}")

    shutil.copytree(ROOT / "llvm/SeaBird", checkout / "llvm/lib/Target/SeaBird",
                    dirs_exist_ok=True)
    shutil.copy2(ROOT / "clang/SeaBird.h", clang_targets / "SeaBird.h")
    shutil.copy2(ROOT / "clang/SeaBird.cpp", clang_targets / "SeaBird.cpp")

    for patch in PATCHES:
        if git(checkout, "apply", "--reverse", "--check", str(patch),
               check=False).returncode == 0:
            print(f"already applied: {patch.name}")
            continue
        probe = git(checkout, "apply", "--check", str(patch), check=False)
        if probe.returncode != 0:
            raise SystemExit(
                f"patch neither applies cleanly nor is already applied: {patch}\n"
                f"{probe.stderr.strip()}"
            )
        git(checkout, "apply", str(patch))
        print(f"applied: {patch.name}")
    print("installed SeaBird LLVM backend and Clang target sources")


if __name__ == "__main__":
    main()
