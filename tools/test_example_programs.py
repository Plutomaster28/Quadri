#!/usr/bin/env python3
"""Execute and validate every SeaBird console example."""

import argparse
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

CASES = {
    "add-two-numbers": [
        (("12", "30"), "First number: Second number: Result: 42\n"),
        (("-8", "3"), "First number: Second number: Result: -5\n"),
    ],
    "simple-calculator": [
        (("20", "+", "3"), "First number: Operator (+ - * /): Second number: Result: 23\n"),
        (("20", "-", "3"), "First number: Operator (+ - * /): Second number: Result: 17\n"),
        (("20", "*", "3"), "First number: Operator (+ - * /): Second number: Result: 60\n"),
        (("20", "/", "4"), "First number: Operator (+ - * /): Second number: Result: 5\n"),
        (("20", "/", "0"), "First number: Operator (+ - * /): Second number: Division by zero\n"),
    ],
    "count-to-n": [
        (("5",), "N: 1 2 3 4 5\n"),
    ],
    "factorial": [
        (("6",), "N (0-10): Factorial: 720\n"),
        (("11",), "N (0-10): Out of range\n"),
    ],
    "number-guessing-game": [
        (("10", "50", "42"), "Guess: Too low\nGuess: Too high\nGuess: Correct\n"),
    ],
    "even-or-odd": [
        (("18",), "Number: Even\n"),
        (("17",), "Number: Odd\n"),
    ],
    "bmi-calculator": [
        (("180", "50"), "Height in cm: Weight in kg: BMI: 15 Category: Under\n"),
        (("180", "75"), "Height in cm: Weight in kg: BMI: 23 Category: Normal\n"),
        (("180", "100"), "Height in cm: Weight in kg: BMI: 30 Category: Over\n"),
    ],
    "array-sum-find-max": [
        ((), "Sum: 53 Max: 19\n"),
    ],
    "password-checker": [
        (("7319",), "Password: Access Granted\n"),
        (("1234",), "Password: Access Denied\n"),
    ],
    "rock-paper-scissors": [
        (("2",), "Choose 1=rock 2=paper 3=scissors: Computer: 2 Result: Draw\n"),
        (("3",), "Choose 1=rock 2=paper 3=scissors: Computer: 2 Result: You win\n"),
        (("1",), "Choose 1=rock 2=paper 3=scissors: Computer: 2 Result: Computer wins\n"),
    ],
}


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_ihex(path):
    saw_eof = False
    image = bytearray()
    for line in path.read_text(encoding="ascii").splitlines():
        if not line.startswith(":"):
            raise SystemExit(f"{path}: malformed Intel HEX record")
        record = bytes.fromhex(line[1:])
        if len(record) != record[0] + 5 or sum(record) & 0xFF:
            raise SystemExit(f"{path}: invalid Intel HEX checksum")
        if record[3] == 1:
            saw_eof = True
        elif record[3] == 0:
            address = (record[1] << 8) | record[2]
            if address != len(image):
                raise SystemExit(f"{path}: non-contiguous Intel HEX image")
            image.extend(record[4:-1])
    if not saw_eof:
        raise SystemExit(f"{path}: missing Intel HEX EOF record")
    return bytes(image)


def read_hex_dump(path):
    image = bytearray()
    for line in path.read_text(encoding="ascii").splitlines():
        left = line.split("|", 1)[0]
        address_text, octets = left.split(":", 1)
        if int(address_text, 16) != len(image):
            raise SystemExit(f"{path}: non-contiguous readable hex dump")
        image.extend(int(octet, 16) for octet in octets.split())
    return bytes(image)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path,
                        default=ROOT / "build/example-programs")
    args = parser.parse_args()
    artifacts = args.artifacts.resolve()
    manifest = json.loads((artifacts / "manifest.json").read_text())

    executed = 0
    for name, cases in CASES.items():
        directory = artifacts / name
        binary = directory / f"{name}.bin"
        for suffix in (".elf", ".bin", ".hex", ".ihex", ".o", ".s", ".ll",
                       ".disasm.txt", ".object.txt"):
            path = directory / f"{name}{suffix}"
            if not path.is_file() or path.stat().st_size == 0:
                raise SystemExit(f"{name}: missing or empty artifact {path.name}")
            recorded = manifest["programs"][name][path.name]["sha256"]
            if sha256(path) != recorded:
                raise SystemExit(f"{name}: manifest mismatch for {path.name}")
        binary_data = binary.read_bytes()
        elf_data = (directory / f"{name}.elf").read_bytes()
        if (elf_data[:6] != b"\x7fELF\x02\x01" or
                int.from_bytes(elf_data[16:18], "little") != 2 or
                int.from_bytes(elf_data[18:20], "little") != 0x5342):
            raise SystemExit(f"{name}: linked file is not an executable SeaBird ELF64")
        if read_hex_dump(directory / f"{name}.hex") != binary_data:
            raise SystemExit(f"{name}: readable hex dump does not match binary")
        if verify_ihex(directory / f"{name}.ihex") != binary_data:
            raise SystemExit(f"{name}: Intel HEX image does not match binary")

        for inputs, expected in cases:
            result = subprocess.run(
                (str(args.reference), "--console", str(binary), *inputs),
                check=True, text=True, capture_output=True)
            if result.stdout != expected:
                raise SystemExit(
                    f"{name} {inputs}: output mismatch\n"
                    f"expected: {expected!r}\nactual:   {result.stdout!r}")
            executed += 1
        print(f"{name}: {len(cases)} execution case(s) passed")

    print(f"example suite passed: {len(CASES)} programs, {executed} executions")


if __name__ == "__main__":
    main()
