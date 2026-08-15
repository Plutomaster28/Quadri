#!/usr/bin/env python3
"""Native Linux qualification sweep for the SeaBird LLVM/Clang backend."""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT / "tests/llvm"
TOOLS = ("clang", "llc", "llvm-mc", "llvm-readobj", "llvm-objdump",
         "llvm-nm", "llvm-ar", "llvm-ranlib", "llvm-objcopy", "llvm-strip")


def run(*args, capture=False, expect_failure=False):
    result = subprocess.run(tuple(str(arg) for arg in args), cwd=ROOT, text=True,
                            stdout=subprocess.PIPE if capture else None,
                            stderr=subprocess.STDOUT if capture else None)
    if expect_failure:
        if result.returncode == 0:
            raise SystemExit(f"command unexpectedly succeeded: {' '.join(map(str, args))}")
    elif result.returncode:
        raise SystemExit(f"command failed ({result.returncode}): {' '.join(map(str, args))}")
    return result.stdout or ""


def contains(text, *needles):
    for needle in needles:
        if needle not in text:
            raise SystemExit(f"qualification output is missing {needle!r}")


def profile(path: Path):
    if (path.name.startswith("tritium-") or path.name == "c-native-tritium.c" or
            path.name == "varargs-sb32.ll"):
        return "seabird32-unknown-none", "tritium-v1"
    if path.name in ("windowing.s", "windowed-abi.ll"):
        return "seabird32-unknown-none", "axium-m-v1"
    return "seabird64-unknown-none", ""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--llvm-build", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=ROOT / "build/llvm-tests")
    args = parser.parse_args()
    bindir = args.llvm_build.resolve() / "bin"
    tools = {name: bindir / name for name in TOOLS}
    for name, path in tools.items():
        if not path.is_file():
            raise SystemExit(f"missing required tool {name}: {path}")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    clang, llc, mc = tools["clang"], tools["llc"], tools["llvm-mc"]

    version = run(mc, "--version", capture=True)
    contains(version, "seabird32 - SeaBird 32-bit", "seabird64 - SeaBird 64-bit")
    llc_version = run(llc, "--version", capture=True)
    contains(llc_version, "seabird32 - SeaBird 32-bit", "seabird64 - SeaBird 64-bit")
    macros = run(clang, "-target", "seabird32-unknown-none", "-mcpu=axium-m-v1",
                 "-dM", "-E", "-x", "c", "/dev/null", capture=True)
    contains(macros, "#define __SEABIRD_AXIUM_M__ 1", "#define __SEABIRD_PAE32__ 1",
             "#define __SEABIRD_REGISTER_WINDOWS__ 1")

    negative = run(mc, "-triple=seabird64-unknown-none",
                   FIXTURES / "performance-markers-invalid.s", capture=True,
                   expect_failure=True)
    contains(negative, "performance marker is not applicable")
    negative = run(mc, "-triple=seabird32-unknown-none", "-mcpu=tritium-v1",
                   FIXTURES / "tritium-excluded.s", capture=True, expect_failure=True)
    contains(negative, "instruction requires an unavailable feature")

    assembly_count = 0
    excluded = {"performance-markers-invalid.s", "tritium-excluded.s"}
    for source in sorted(FIXTURES.glob("*.s")):
        if source.name in excluded:
            continue
        triple, cpu = profile(source)
        obj = out / f"fixture-{source.stem}.o"
        cpu_arg = (f"-mcpu={cpu}",) if cpu else ()
        run(mc, f"-triple={triple}", *cpu_arg, "-filetype=obj", source, "-o", obj)
        run(tools["llvm-objdump"], "-d", obj, capture=True)
        assembly_count += 1

    showcase_profiles = (
        (ROOT / "examples/isa-showcase/seabird64-showcase.s",
         "seabird64-unknown-none", ""),
        (ROOT / "examples/isa-showcase/axium-pae-window-showcase.s",
         "seabird32-unknown-none", "axium-m-v1"),
        (ROOT / "examples/isa-showcase/tritium-showcase.s",
         "seabird32-unknown-none", "tritium-v1"),
    )
    showcase_objects = {}
    for source, triple, cpu in showcase_profiles:
        obj = out / f"showcase-{source.stem}.o"
        cpu_arg = (f"-mcpu={cpu}",) if cpu else ()
        run(mc, f"-triple={triple}", *cpu_arg, "-filetype=obj", source,
            "-o", obj)
        run(tools["llvm-objdump"], "-d", obj, capture=True)
        showcase_objects[source.stem] = obj
        assembly_count += 1

    window_headers = run(tools["llvm-readobj"], "--file-headers",
                         out / "fixture-windowing.o", capture=True)
    contains(window_headers, "Format: elf32-seabird", "Machine: 0x5342",
             "Flags [ (0x3)")
    external_headers = run(tools["llvm-readobj"], "--file-headers", "--relocations",
                           out / "fixture-external-reloc.o", capture=True)
    contains(external_headers, "Format: elf64-seabird", "R_SB_PCREL32",
             "R_SB_ABS16", "R_SB_ABS32", "R_SB_ABS64")

    ir_count = 0
    for source in sorted(FIXTURES.glob("*.ll")):
        triple, cpu = profile(source)
        assembly, obj = out / f"{source.stem}.s", out / f"{source.stem}.o"
        cpu_arg = (f"-mcpu={cpu}",) if cpu else ()
        run(llc, f"-mtriple={triple}", *cpu_arg, "-O2", "-filetype=asm", source,
            "-o", assembly)
        run(mc, f"-triple={triple}", *cpu_arg, "-filetype=obj", assembly, "-o", obj)
        ir_count += 1

    c_count = 0
    for source in sorted(FIXTURES.glob("*.c")):
        triple, cpu = profile(source)
        assembly, obj = out / f"{source.stem}.s", out / f"{source.stem}.o"
        cpu_arg = (f"-mcpu={cpu}",) if cpu else ()
        common = (clang, "-target", triple, *cpu_arg, "-O2", "-ffreestanding",
                  "-fno-builtin", "-fno-ident", source)
        run(*common, "-S", "-o", assembly)
        run(*common, "-c", "-o", obj)
        roundtrip = out / f"{source.stem}-roundtrip.o"
        run(mc, f"-triple={triple}", *cpu_arg, "-filetype=obj", assembly,
            "-o", roundtrip)
        run(tools["llvm-objcopy"], "-O", "binary", "--only-section=.text", obj,
            out / f"{source.stem}.bin")
        c_count += 1

    utility_obj = out / "c-native-fp128.o"
    symbols = run(tools["llvm-nm"], utility_obj, capture=True)
    contains(symbols, "sb_quad_math", "sb_native_fp128_wrapper")
    archive = out / "libseabird-fp128.a"
    run(tools["llvm-ar"], "rcs", archive, utility_obj)
    run(tools["llvm-ranlib"], archive)
    contains(run(tools["llvm-ar"], "t", archive, capture=True), utility_obj.name)
    raw, ihex = out / "fp128.raw", out / "fp128.hex"
    run(tools["llvm-objcopy"], "-O", "binary", utility_obj, raw)
    run(tools["llvm-objcopy"], "-O", "ihex", utility_obj, ihex)
    if not raw.stat().st_size or not ihex.read_text().startswith(":"):
        raise SystemExit("object conversion utilities produced invalid output")
    stripped = out / "fp128-stripped.o"
    run(tools["llvm-strip"], "--strip-debug", utility_obj, "-o", stripped)
    contains(run(tools["llvm-readobj"], "--file-headers", stripped, capture=True),
             "Format: elf64-seabird")

    linked = out / "c-linked.bin"
    run(sys.executable, ROOT / "tools/link_seabird.py", "-o", linked,
        out / "c-link-caller.o", out / "c-link-callee.o")
    if not linked.stat().st_size:
        raise SystemExit("SeaBird static linker produced an empty image")
    for name in ("c-ordered", "c-stack-args"):
        run(sys.executable, ROOT / "tools/link_seabird.py", "-o",
            out / f"{name}-linked.bin", out / f"{name}.o")

    tritium_elf = out / "tritium-linked.elf"
    run(sys.executable, ROOT / "tools/link_seabird.py", "--format", "elf",
        "--entry", "tritium_native_call", "-o", tritium_elf,
        out / "c-native-tritium.o")
    contains(run(tools["llvm-readobj"], "--file-headers", "--program-headers",
                 tritium_elf, capture=True),
             "Format: elf32-seabird", "ProgramHeaderCount: 1",
             "Flags [ (0x0)")
    axium_elf = out / "axium-linked.elf"
    run(sys.executable, ROOT / "tools/link_seabird.py", "--format", "elf",
        "--entry", "window_call8", "-o", axium_elf,
        out / "windowed-abi.o")
    contains(run(tools["llvm-readobj"], "--file-headers", "--program-headers",
                 axium_elf, capture=True),
             "Format: elf32-seabird", "Flags [ (0x3)",
             "ProgramHeaderCount: 1")
    mixed = run(sys.executable, ROOT / "tools/link_seabird.py", "-o",
                out / "invalid-mixed-class.bin", out / "c-smoke.o",
                out / "c-native-tritium.o", capture=True, expect_failure=True)
    contains(mixed, "cannot mix SeaBird ELF classes")
    for stem, entry, expected_format, expected_flags in (
        ("seabird64-showcase", "seabird64_showcase", "elf64-seabird", "0x0"),
        ("axium-pae-window-showcase", "axium_pae_window_showcase",
         "elf32-seabird", "0x3"),
        ("tritium-showcase", "tritium_showcase", "elf32-seabird", "0x0"),
    ):
        executable = out / f"{stem}.elf"
        run(sys.executable, ROOT / "tools/link_seabird.py", "--format", "elf",
            "--entry", entry, "-o", executable, showcase_objects[stem])
        contains(run(tools["llvm-readobj"], "--file-headers",
                     "--program-headers", executable, capture=True),
                 f"Format: {expected_format}", f"Flags [ ({expected_flags})")

    runtime_objects = []
    for name, source in (("crt0", ROOT / "runtime/crt0.c"),
                         ("string", ROOT / "runtime/string.c")):
        obj = out / f"{name}.o"
        run(clang, "-target", "seabird64-unknown-none", "-O2", "-ffreestanding",
            "-fno-builtin", "-fno-ident", "-c", source, "-o", obj)
        runtime_objects.append(obj)
    syscalls = out / "syscalls.o"
    run(mc, "-triple=seabird64-unknown-none", "-filetype=obj",
        ROOT / "runtime/syscalls.s", "-o", syscalls)
    run(sys.executable, ROOT / "tools/link_seabird.py", "--format", "elf",
        "--entry", "_start", "-o", out / "hosted.elf", runtime_objects[0],
        out / "hosted-main.o", syscalls)
    run(sys.executable, ROOT / "tools/link_seabird.py", "--format", "elf",
        "--entry", "_start", "-o", out / "hosted-libc.elf", runtime_objects[0],
        out / "hosted-libc-main.o", runtime_objects[1])

    run(sys.executable, ROOT / "tools/build_example_programs.py",
        "--llvm-build", args.llvm_build.resolve())
    print(f"native LLVM qualification passed: {assembly_count} assembly fixtures/demos, "
          f"{ir_count} IR fixtures, {c_count} native C fixtures, utilities/link/examples")


if __name__ == "__main__":
    main()
