#!/usr/bin/env python3
"""Build and run the executable SeaBird ratification checks."""

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PYTHON = sys.executable


def run(*args):
    subprocess.run(args, cwd=ROOT, check=True)


def main():
    vectors = json.loads((ROOT / "tests/ratification-vectors.json").read_text())
    expected_priority = [
        "INVALID_ENCODING", "UNAVAILABLE_FEATURE", "ILLEGAL_MODE", "PRIVILEGE",
        "NON_CANONICAL", "ALIGNMENT", "PAGE_TRANSLATION", "ARITHMETIC", "DEBUG_TRAP"
    ]
    if vectors["exception_priority"] != expected_priority:
        raise SystemExit("exception priority vector does not match Volume 2")
    if len({case["name"] for case in vectors["memory_litmus"]}) != len(vectors["memory_litmus"]):
        raise SystemExit("duplicate memory litmus name")

    run(PYTHON, "tools/build_isa_spec.py")
    run(PYTHON, "tools/generate_golden_vectors.py")
    run(PYTHON, "tools/verify_ratification.py")
    run(PYTHON, "tools/independent_oracle.py")
    run("g++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-pedantic", "src/seabird_ref.cpp", "-o", "seabird-ref.exe")
    run(str(ROOT / "seabird-ref.exe"), "--self-test")
    varargs_object = ROOT / "build/llvm-tests/varargs.o"
    if varargs_object.exists():
        from link_seabird import link
        image, definitions, _ = link([varargs_object])
        varargs_binary = ROOT / "build/llvm-tests/varargs-linked.bin"
        varargs_binary.write_bytes(image)
        run(str(ROOT / "seabird-ref.exe"), "--expect-result",
            str(varargs_binary), hex(definitions["call_sum_three"]), "61")
        run(str(ROOT / "seabird-ref.exe"), "--expect-result",
            str(varargs_binary), hex(definitions["call_sum_two_ints"]), "31")
        run(str(ROOT / "seabird-ref.exe"), "--expect-fp-result",
            str(varargs_binary), hex(definitions["call_sum_two_fp"]), "7.0")
    native_varargs_object = ROOT / "build/llvm-tests/c-native-varargs.o"
    if native_varargs_object.exists():
        from link_seabird import link
        image, definitions, _ = link([native_varargs_object])
        native_varargs_binary = ROOT / "build/llvm-tests/c-native-varargs.bin"
        native_varargs_binary.write_bytes(image)
        run(str(ROOT / "seabird-ref.exe"), "--expect-result",
            str(native_varargs_binary),
            hex(definitions["seabird_native_call"]), "61")
        run(str(ROOT / "seabird-ref.exe"), "--expect-fp-result",
            str(native_varargs_binary),
            hex(definitions["seabird_native_fp_call"]), "7.0")
    native_fp32_object = ROOT / "build/llvm-tests/c-native-fp32.o"
    if native_fp32_object.exists():
        from link_seabird import link
        image, definitions, _ = link([native_fp32_object])
        native_fp32_binary = ROOT / "build/llvm-tests/c-native-fp32.bin"
        native_fp32_binary.write_bytes(image)
        run(str(ROOT / "seabird-ref.exe"), "--expect-fp32-result",
            str(native_fp32_binary),
            hex(definitions["seabird_f32_call"]), "3.75")
        run(str(ROOT / "seabird-ref.exe"), "--expect-fp32-result",
            str(native_fp32_binary),
            hex(definitions["seabird_f32_arithmetic_call"]), "7.5")
        run(str(ROOT / "seabird-ref.exe"), "--expect-fp32-result",
            str(native_fp32_binary),
            hex(definitions["seabird_f32_stack_call"]), "55.0")
    native_aggregate_object = ROOT / "build/llvm-tests/c-native-aggregate.o"
    if native_aggregate_object.exists():
        from link_seabird import link
        image, definitions, _ = link([native_aggregate_object])
        native_aggregate_binary = ROOT / "build/llvm-tests/c-native-aggregate.bin"
        native_aggregate_binary.write_bytes(image)
        run(str(ROOT / "seabird-ref.exe"), "--expect-result",
            str(native_aggregate_binary),
            hex(definitions["seabird_pair_call"]), "42")
        run(str(ROOT / "seabird-ref.exe"), "--expect-fp32-result",
            str(native_aggregate_binary),
            hex(definitions["seabird_float_pair_call"]), "3.75")
    native_fp_compare_object = ROOT / "build/llvm-tests/c-native-fp-compare.o"
    if native_fp_compare_object.exists():
        from link_seabird import link
        image, definitions, _ = link([native_fp_compare_object])
        native_fp_compare_binary = ROOT / "build/llvm-tests/c-native-fp-compare.bin"
        native_fp_compare_binary.write_bytes(image)
        run(str(ROOT / "seabird-ref.exe"), "--expect-result",
            str(native_fp_compare_binary),
            hex(definitions["seabird_fp_compare_call"]), "23")
    native_fp_unsigned_object = ROOT / "build/llvm-tests/c-native-fp-unsigned.o"
    if native_fp_unsigned_object.exists():
        from link_seabird import link
        image, definitions, _ = link([native_fp_unsigned_object])
        native_fp_unsigned_binary = ROOT / "build/llvm-tests/c-native-fp-unsigned.bin"
        native_fp_unsigned_binary.write_bytes(image)
        run(str(ROOT / "seabird-ref.exe"), "--expect-result",
            str(native_fp_unsigned_binary),
            hex(definitions["seabird_fp_unsigned_call"]), "4")
    native_fp128_object = ROOT / "build/llvm-tests/c-native-fp128.o"
    if native_fp128_object.exists():
        from link_seabird import link
        image, definitions, _ = link([native_fp128_object])
        native_fp128_binary = ROOT / "build/llvm-tests/c-native-fp128.bin"
        native_fp128_binary.write_bytes(image)
        run(str(ROOT / "seabird-ref.exe"), "--expect-result",
            str(native_fp128_binary),
            hex(definitions["sb_native_fp128_wrapper"]), "4")
    for name in ("c-smoke", "c-call", "c-branch", "c-memory", "c-stack"):
        binary = ROOT / f"build/llvm-tests/{name}.bin"
        if binary.exists():
            run(str(ROOT / "seabird-ref.exe"), f"--{name}", str(binary))
    linked_binary = ROOT / "build/llvm-tests/c-linked.bin"
    if linked_binary.exists():
        run(str(ROOT / "seabird-ref.exe"), "--c-linked", str(linked_binary))
    extra = {
        "c-indirect": "c-indirect.bin",
        "c-ordered": "c-ordered-linked.bin",
        "c-stack-args": "c-stack-args-linked.bin",
        "c-fp-vector": "c-fp-vector.bin",
        "c-fp-memory": "c-fp-memory.bin",
        "c-vector-memory": "c-vector-memory.bin",
        "c-fp-convert": "c-fp-convert.bin",
    }
    for mode, filename in extra.items():
        binary = ROOT / "build/llvm-tests" / filename
        if binary.exists():
            run(str(ROOT / "seabird-ref.exe"), f"--{mode}", str(binary))
    hosted = ROOT / "build/llvm-tests/hosted.elf"
    if hosted.exists():
        run(str(ROOT / "seabird-ref.exe"), "--elf", str(hosted))
    hosted_libc = ROOT / "build/llvm-tests/hosted-libc.elf"
    if hosted_libc.exists():
        run(str(ROOT / "seabird-ref.exe"), "--elf", str(hosted_libc))
    run("g++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-pedantic", "src/main.cpp", "-o", "pebble-xlate.exe")
    output = subprocess.check_output([
        str(ROOT / "pebble-xlate.exe"), "jit",
        "49 b8 88 77 66 55 44 33 22 11 4d 01 c8 c3"
    ], cwd=ROOT, text=True)
    for required in ("FE 80 01 C0 04 88 77 66 55 44 33 22 11", "FE 80 20 C1 05", "60"):
        if required not in output:
            raise SystemExit(f"translator output missing expected bytes: {required}")
    print(f"conformance passed: {len(vectors['decoder'])} decoder vectors, {len(vectors['memory_litmus'])} memory litmus contracts")


if __name__ == "__main__":
    main()
