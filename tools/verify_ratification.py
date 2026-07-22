#!/usr/bin/env python3
"""Fail the build when a SeaBird ratification invariant is violated."""

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ISA = json.loads((ROOT / "spec/seabird-isa.json").read_text())
LAYOUTS = json.loads((ROOT / "spec/architectural-layouts.json").read_text())
GOLDEN_PATH = ROOT / "tests/golden-vectors.json"
errors = []


def fail(message):
    errors.append(message)


def unique_values(name, mapping):
    values = list(mapping.values())
    if len(values) != len(set(values)):
        fail(f"{name}: duplicate numeric assignment")


def validate_fields(name, spec):
    occupied = set()
    for field, (offset, size) in spec["fields"].items():
        if offset < 0 or size <= 0 or offset + size > spec["size"]:
            fail(f"{name}.{field}: outside structure")
        bits = set(range(offset, offset + size))
        if occupied & bits:
            fail(f"{name}.{field}: overlaps another field")
        occupied |= bits
    if spec["size"] % spec["alignment"]:
        fail(f"{name}: size is not a multiple of alignment")


def validate_bitfields(name, spec):
    used = set()
    for field, (bit, width) in spec["fields"].items():
        bits = set(range(bit, bit + width))
        if bit + width > spec["width"] or used & bits:
            fail(f"{name}.{field}: invalid or overlapping bitfield")
        used |= bits


if any(x["status"] == "provisional" for x in ISA["instructions"]):
    fail("instruction database contains provisional entries")

if not GOLDEN_PATH.exists():
    fail("golden-vector corpus is missing")
else:
    golden = json.loads(GOLDEN_PATH.read_text())
    expected_ids = {x["id"] for x in ISA["instructions"] if x["status"] == "normative"}
    vector_ids = [x["instruction_id"] for x in golden["vectors"]]
    if len(vector_ids) != len(set(vector_ids)):
        fail("golden-vector corpus contains duplicate instruction IDs")
    if expected_ids != set(vector_ids):
        fail("golden-vector coverage does not exactly match normative instruction IDs")

opcodes = {}
for inst in ISA["instructions"]:
    op = tuple(inst["encoding"]["opcode"])
    if op:
        if op in opcodes:
            fail(f"opcode collision: {opcodes[op]} and {inst['syntax']}")
        opcodes[op] = inst["syntax"]
    if not inst["encoding"].get("operand_binding"):
        fail(f"{inst['syntax']}: missing operand binding")

banned = ("immediate_field(", "according to the description", "_TRANSFORM(", "implicit packed")
for inst in ISA["instructions"]:
    if inst["status"] == "normative":
        operation = " ".join(inst["operation"])
        for token in banned:
            if token in operation:
                fail(f"{inst['syntax']}: undefined semantic placeholder {token}")

for group, bits in LAYOUTS["feature_bits"].items():
    unique_values(f"feature_bits.{group}", bits)
for name, spec in LAYOUTS["control_registers"].items():
    validate_bitfields(name, spec)
for name, spec in LAYOUTS["structures"].items():
    validate_fields(name, spec)
unique_values("ELF relocations", LAYOUTS["elf"]["relocations"])

if errors:
    print("RATIFICATION CHECK FAILED", file=sys.stderr)
    for error in errors:
        print(f"- {error}", file=sys.stderr)
    sys.exit(1)

print(f"ratification checks passed: {len(ISA['instructions'])} entries, {len(opcodes)} encoded allocations")
