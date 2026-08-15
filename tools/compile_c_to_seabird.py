#!/usr/bin/env python3
"""Compile C through SeaBird LLVM IR, assembly, ELF object, and raw text."""

import argparse
import subprocess
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("--output-prefix", type=Path)
    parser.add_argument("--llvm-build", type=Path, required=True)
    parser.add_argument("--target", default="seabird64-unknown-none")
    parser.add_argument("--cpu", default="")
    args = parser.parse_args()
    source = args.input.resolve()
    prefix = (args.output_prefix or source.with_suffix("")).resolve()
    prefix.parent.mkdir(parents=True, exist_ok=True)
    bindir = args.llvm_build.resolve() / "bin"
    clang, llc = bindir / "clang", bindir / "llc"
    mc, objcopy = bindir / "llvm-mc", bindir / "llvm-objcopy"
    for tool in (clang, llc, mc, objcopy):
        if not tool.is_file():
            raise SystemExit(f"missing tool: {tool}")
    cpu = (f"-mcpu={args.cpu}",) if args.cpu else ()
    ir, assembly = prefix.with_suffix(".ll"), prefix.with_suffix(".s")
    obj, binary = prefix.with_suffix(".o"), prefix.with_suffix(".bin")
    subprocess.run((clang, "-target", args.target, *cpu, "-O2", "-ffreestanding",
                    "-fno-builtin", "-fno-ident", "-S", "-emit-llvm", source,
                    "-o", ir), check=True)
    subprocess.run((llc, f"-mtriple={args.target}", *cpu, "-O2", "-filetype=asm",
                    ir, "-o", assembly), check=True)
    subprocess.run((mc, f"-triple={args.target}", *cpu, "-filetype=obj", assembly,
                    "-o", obj), check=True)
    subprocess.run((objcopy, "-O", "binary", "--only-section=.text", obj, binary),
                   check=True)
    print(f"LLVM IR: {ir}\nAssembly: {assembly}\nELF object: {obj}\nRaw text: {binary}")


if __name__ == "__main__":
    main()
