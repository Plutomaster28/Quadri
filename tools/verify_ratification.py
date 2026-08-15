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

if ISA["architecture_version"] != LAYOUTS["architecture_version"]:
    fail("ISA and architectural-layout versions differ")

markers = ISA.get("performance_markers", {})
if markers.get("escape") != 0xFD:
    fail("performance marker escape is not FD")
marker_entries = markers.get("markers", [])
if [x.get("id") for x in marker_entries] != list(range(1, 11)):
    fail("performance marker IDs must be the contiguous range 1..10")
if len({x.get("name") for x in marker_entries}) != len(marker_entries):
    fail("performance marker names are not unique")
if {x.get("name") for x in marker_entries[-2:]} != {"reuse", "leaf"}:
    fail("register-window call markers are missing")

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
if (markers.get("escape"),) in opcodes:
    fail("performance marker escape collides with an instruction opcode")

banned = ("immediate_field(", "according to the description", "_TRANSFORM(", "implicit packed")
for inst in ISA["instructions"]:
    if inst["status"] == "normative":
        operation = " ".join(inst["operation"])
        for token in banned:
            if token in operation:
                fail(f"{inst['syntax']}: undefined semantic placeholder {token}")

window_insts = {x["mnemonic"]: x for x in ISA["instructions"]
                if x.get("feature") == "WINDOW" and x["status"] == "normative"}
if set(window_insts) != {"WINNEW", "WINPREV", "WINRESERVE", "WINPIN", "WINRELEASE"}:
    fail("register-window instruction surface is incomplete")
for mnemonic in ("WINNEW", "WINPREV"):
    if "spill or restore" not in window_insts.get(mnemonic, {}).get("memory_order", ""):
        fail(f"{mnemonic}: transparent memory side effects are not modeled")

for group, bits in LAYOUTS["feature_bits"].items():
    unique_values(f"feature_bits.{group}", bits)
for name, spec in LAYOUTS["control_registers"].items():
    validate_bitfields(name, spec)
for name, spec in LAYOUTS.get("system_register_layouts", {}).items():
    validate_bitfields(f"system_register_layouts.{name}", spec)
for name, spec in LAYOUTS["structures"].items():
    validate_fields(name, spec)
pae = LAYOUTS.get("translation_regimes", {}).get("PAE32", {})
if pae:
    validate_bitfields("translation_regimes.PAE32.pte", {
        "width": pae["pte_bits"], "fields": pae["pte_fields"]})
    indices = pae["indices"]
    covered = set()
    for field, (bit, width) in indices.items():
        bits = set(range(bit, bit + width))
        if bit + width > pae["virtual_bits"] or covered & bits:
            fail(f"translation_regimes.PAE32.{field}: invalid or overlapping VA field")
        covered |= bits
    if covered != set(range(pae["virtual_bits"])):
        fail("translation_regimes.PAE32: VA fields do not cover exactly 32 bits")
    if pae["large_page_bytes"] != (1 << indices["L2"][0]):
        fail("translation_regimes.PAE32: large-page size does not match L2 coverage")
    if pae.get("memory_types") != {"WB": 0, "WT": 1, "UC": 2, "DEVICE": 3}:
        fail("translation_regimes.PAE32: two-bit memory-type encoding is incomplete")
windows = LAYOUTS.get("register_windows", {})
if windows:
    ranges = [windows[name] for name in ("global", "incoming", "local", "outgoing")]
    flattened = [reg for first, last in ranges for reg in range(first, last + 1)]
    if flattened != list(range(windows["visible_registers"])):
        fail("register_windows: register classes must partition R0-R31")
    first, last = windows.get("spill_register_order", [-1, -1])
    if (first, last) != (8, 31):
        fail("register_windows: spill records must contain R8-R31 in order")
    pointer_bytes = {"Clownfish": 2, "Tetra": 4, "Dragonet": 8, "Droplet": 16}
    for mode, size in windows["spill_payload_bytes_by_mode"].items():
        if size != 24 * pointer_bytes[mode]:
            fail(f"register_windows: {mode} spill payload is not R8-R31")
        stride = windows["spill_record_bytes_by_mode"][mode]
        if stride < size or stride % windows["spill_alignment"]:
            fail(f"register_windows: {mode} spill stride is invalid")
unique_values("ELF relocations", LAYOUTS["elf"]["relocations"])
unique_values("ELF flags", LAYOUTS["elf"].get("flags", {}))

if errors:
    print("RATIFICATION CHECK FAILED", file=sys.stderr)
    for error in errors:
        print(f"- {error}", file=sys.stderr)
    sys.exit(1)

print(f"ratification checks passed: {len(ISA['instructions'])} entries, {len(opcodes)} encoded allocations")
