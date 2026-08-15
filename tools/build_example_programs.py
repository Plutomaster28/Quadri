#!/usr/bin/env python3
"""Compile the SeaBird example-program suite into inspectable release artifacts."""

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ROOT / "examples" / "programs"
PROGRAMS = (
    "add-two-numbers",
    "simple-calculator",
    "count-to-n",
    "factorial",
    "number-guessing-game",
    "even-or-odd",
    "bmi-calculator",
    "array-sum-find-max",
    "password-checker",
    "rock-paper-scissors",
)


def run(command, *, stdout=None):
    subprocess.run([str(part) for part in command], cwd=ROOT, check=True,
                   stdout=stdout)


def tool(bin_dir, name):
    for candidate in (bin_dir / name, bin_dir / f"{name}.exe"):
        if candidate.exists():
            return candidate
    raise SystemExit(f"missing {name} in {bin_dir}")


def write_hex_dump(data, destination):
    lines = []
    for offset in range(0, len(data), 16):
        chunk = data[offset:offset + 16]
        octets = " ".join(f"{byte:02x}" for byte in chunk)
        text = "".join(chr(byte) if 32 <= byte < 127 else "." for byte in chunk)
        lines.append(f"{offset:08x}: {octets:<47}  |{text}|")
    destination.write_text("\n".join(lines) + "\n", encoding="ascii")


def ihex_record(address, record_type, payload):
    body = bytes((len(payload), (address >> 8) & 0xFF, address & 0xFF,
                  record_type)) + payload
    checksum = (-sum(body)) & 0xFF
    return ":" + (body + bytes((checksum,))).hex().upper()


def write_intel_hex(data, destination):
    lines = [
        ihex_record(offset, 0, data[offset:offset + 16])
        for offset in range(0, len(data), 16)
    ]
    lines.append(ihex_record(0, 1, b""))
    destination.write_text("\n".join(lines) + "\n", encoding="ascii")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--llvm-build", type=Path, required=True,
                        help="qualified LLVM build or unpacked SDK root")
    parser.add_argument("--output", type=Path,
                        default=ROOT / "build" / "example-programs")
    parser.add_argument("--opt", default="-O2",
                        choices=("-O0", "-O1", "-O2", "-O3", "-Os"))
    args = parser.parse_args()

    llvm_root = args.llvm_build.resolve()
    bin_dir = llvm_root / "bin"
    clang = tool(bin_dir, "clang")
    llvm_mc = tool(bin_dir, "llvm-mc")
    llvm_objdump = tool(bin_dir, "llvm-objdump")
    llvm_readobj = tool(bin_dir, "llvm-readobj")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)

    console_object = output / "console.o"
    run((llvm_mc, "-triple=seabird64-unknown-none", "-filetype=obj",
         ROOT / "runtime/console.s", "-o", console_object))

    manifest = {
        "target": "seabird64-unknown-none",
        "optimization": args.opt,
        "programs": {},
    }
    for name in PROGRAMS:
        source = SOURCES / f"{name}.c"
        directory = output / name
        directory.mkdir(parents=True, exist_ok=True)
        ir = directory / f"{name}.ll"
        assembly = directory / f"{name}.s"
        object_file = directory / f"{name}.o"
        elf = directory / f"{name}.elf"
        binary = directory / f"{name}.bin"
        hex_dump = directory / f"{name}.hex"
        intel_hex = directory / f"{name}.ihex"
        disassembly = directory / f"{name}.disasm.txt"
        object_report = directory / f"{name}.object.txt"

        common = (
            clang, "--target=seabird64-unknown-none", args.opt,
            "-ffreestanding", "-fno-builtin", "-fno-ident",
            "-I", ROOT / "runtime", source,
        )
        run((*common, "-S", "-emit-llvm", "-o", ir))
        run((*common, "-S", "-o", assembly))
        run((*common, "-c", "-o", object_file))
        run((sys.executable, ROOT / "tools/link_seabird.py", "--format=elf",
             "--entry=main", "-o", elf, object_file, console_object))
        run((sys.executable, ROOT / "tools/link_seabird.py", "--format=binary",
             "-o", binary, object_file, console_object))
        with disassembly.open("w", encoding="utf-8") as stream:
            run((llvm_objdump, "-dr", object_file), stdout=stream)
        with object_report.open("w", encoding="utf-8") as stream:
            run((llvm_readobj, "--file-headers", "--sections", "--symbols",
                 "--relocations", object_file), stdout=stream)

        data = binary.read_bytes()
        write_hex_dump(data, hex_dump)
        write_intel_hex(data, intel_hex)
        artifacts = (ir, assembly, object_file, elf, binary, hex_dump,
                     intel_hex, disassembly, object_report)
        manifest["programs"][name] = {
            path.name: {"bytes": path.stat().st_size, "sha256": digest(path)}
            for path in artifacts
        }
        print(f"{name}: {len(data)} linked bytes")

    manifest_path = output / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n",
                             encoding="utf-8")
    print(f"Artifacts: {output}")
    print(f"Manifest:  {manifest_path}")


if __name__ == "__main__":
    main()
