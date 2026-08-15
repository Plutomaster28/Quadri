#!/usr/bin/env python3
"""Generate one deterministic ratification vector for every normative instruction."""

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DB = json.loads((ROOT / "spec/seabird-isa.json").read_text())

EVENT_CATEGORIES = {
    "Architectural System Extensions": "system",
    "Atomic & Synchronization": "atomic",
    "Memory / Addressing Extensions": "memory",
    "Stack & Call Frame": "stack",
    "System & Privileged": "system",
    "Transactional Memory": "transaction"
}
MEMORY_MNEMONICS = {"LD", "ST", "LDI", "LDB", "LDH", "LDW", "LDQ", "STB", "STH", "STW", "STQ", "LDP", "STP"}
BRANCHES = {"JMP", "JMPA", "CALL", "CALLA", "RET", "JE", "JNE", "JG", "JGE", "JL", "JLE", "JC", "JNC", "JO", "JNO", "JS", "JNS", "JZR", "JNZR", "BRR", "TRAP", "YIELD"}
FP = {"FADD", "FSUB", "FMUL", "FDIV", "FSQRT", "FCMP", "FCVTI", "FCVTS", "FCVTU", "FCVTUS", "FNEG", "FABS", "FMADD", "FMSUB", "FNMADD", "FNMSUB", "FMIN", "FMAX", "FRECIP", "FRSQRT", "FRND", "FRNDZ", "FCVT.S2D", "FCVT.D2S", "FCVTINT", "FCLASS", "FCHS", "FTEST"}


def expected(inst):
    m, category = inst["mnemonic"], inst["category"]
    if m in BRANCHES:
        return {"kind": "control", "event": m, "next_ip": 0x1004, "target": 0x1010}
    if m in MEMORY_MNEMONICS:
        return {"kind": "event", "event": "memory:" + m}
    if category in EVENT_CATEGORIES:
        return {"kind": "event", "event": EVENT_CATEGORIES[category] + ":" + m}
    if m in {"DIV", "DIVI", "UDIV", "MOD", "MODI"}:
        return {"kind": "fault", "fault": "DIV_ZERO"}
    if m in {"CLZ", "CTZ", "LZCNT", "TZCNT", "TZCNTV", "CLZ_FAST", "TZCNT_FAST"}:
        return {"kind": "scalar", "value": 64}
    if m == "BLSMSK":
        return {"kind": "scalar", "value": 0xFFFFFFFFFFFFFFFF}
    if m in FP:
        special = {
            "FDIV": "nan", "FRECIP": "+inf", "FRSQRT": "+inf",
            "FNEG": "-zero", "FCHS": "-zero", "FCLASS": 8,
            "FCMP": {"ZF": 1, "PF": 0, "CF": 0}, "FTEST": 0
        }
        return {"kind": "fp", "value": special.get(m, "+zero")}
    if category in {"SIMD / Vector", "Advanced Vector Extensions"}:
        if m in {"VRECIP_EST", "VRSQRT_EST"}:
            return {"kind": "vector", "lanes": ["+inf"] * 4}
        if m == "VFPCLASS":
            return {"kind": "vector", "lanes": [8, 8, 8, 8]}
        return {"kind": "vector", "lanes": [0, 0, 0, 0]}
    if category == "Cryptographic Extensions":
        if m == "AESENC":
            return {"kind": "bytes", "hex": "63" * 16}
        if m == "AESDEC":
            return {"kind": "bytes", "hex": "52" * 16}
        return {"kind": "bytes", "hex": "00" * 16}
    return {"kind": "scalar", "value": 0}


vectors = []
for inst in DB["instructions"]:
    if inst["status"] != "normative":
        continue
    vectors.append({
        "instruction_id": inst["id"], "mnemonic": inst["mnemonic"],
        "category": inst["category"], "case": "zero-identity-v1",
        "inputs": {"dst": 0, "src": 0, "aux": 0, "imm": 0, "lanes": [0,0,0,0], "flags": {"CF":0,"ZF":0,"SF":0,"OF":0}},
        "expected": expected(inst)
    })

out = {"schema_version": 1, "architecture_version": DB["architecture_version"], "vectors": vectors}
(ROOT / "tests/golden-vectors.json").write_text(json.dumps(out, indent=2) + "\n")
print(f"generated {len(vectors)} golden vectors")
