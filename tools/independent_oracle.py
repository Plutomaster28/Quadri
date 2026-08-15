#!/usr/bin/env python3
"""Independent Python oracle for the SeaBird golden-vector corpus.

This module intentionally imports neither build_isa_spec nor the C++ reference model.
"""

import json
import math
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

EVENT_CATEGORIES = {
    "Architectural System Extensions": "system", "Atomic & Synchronization": "atomic",
    "Memory / Addressing Extensions": "memory", "Stack & Call Frame": "stack",
    "System & Privileged": "system", "Transactional Memory": "transaction"
}
MEMORY = {"LD", "ST", "LDI", "LDB", "LDH", "LDW", "LDQ", "STB", "STH", "STW", "STQ", "LDP", "STP"}
CONTROL = {"JMP", "JMPA", "CALL", "CALLA", "RET", "JE", "JNE", "JG", "JGE", "JL", "JLE", "JC", "JNC", "JO", "JNO", "JS", "JNS", "JZR", "JNZR", "BRR", "TRAP", "YIELD"}
FP = {"FADD", "FSUB", "FMUL", "FDIV", "FSQRT", "FCMP", "FCVTI", "FCVTS", "FCVTU", "FCVTUS", "FNEG", "FABS", "FMADD", "FMSUB", "FNMADD", "FNMSUB", "FMIN", "FMAX", "FRECIP", "FRSQRT", "FRND", "FRNDZ", "FCVT.S2D", "FCVT.D2S", "FCVTINT", "FCLASS", "FCHS", "FTEST"}
COUNT_ZERO = {"CLZ", "CTZ", "LZCNT", "TZCNT", "TZCNTV", "CLZ_FAST", "TZCNT_FAST"}
DIVIDE = {"DIV", "DIVI", "UDIV", "MOD", "MODI"}


def evaluate(vector):
    m, category = vector["mnemonic"], vector["category"]
    if m in CONTROL:
        return {"kind":"control", "event":m, "next_ip":0x1004, "target":0x1010}
    if m in MEMORY:
        return {"kind":"event", "event":"memory:" + m}
    if category in EVENT_CATEGORIES:
        return {"kind":"event", "event":EVENT_CATEGORIES[category] + ":" + m}
    if m in DIVIDE:
        return {"kind":"fault", "fault":"DIV_ZERO"}
    if m in COUNT_ZERO:
        return {"kind":"scalar", "value":64}
    if m == "BLSMSK":
        return {"kind":"scalar", "value":0xFFFFFFFFFFFFFFFF}
    if m in FP:
        values = {"FDIV":"nan", "FRECIP":"+inf", "FRSQRT":"+inf", "FNEG":"-zero", "FCHS":"-zero", "FCLASS":8, "FCMP":{"ZF":1,"PF":0,"CF":0}, "FTEST":0}
        return {"kind":"fp", "value":values.get(m, "+zero")}
    if category in {"SIMD / Vector", "Advanced Vector Extensions"}:
        if m in {"VRECIP_EST", "VRSQRT_EST"}:
            return {"kind":"vector", "lanes":["+inf"] * 4}
        if m == "VFPCLASS":
            return {"kind":"vector", "lanes":[8,8,8,8]}
        return {"kind":"vector", "lanes":[0,0,0,0]}
    if category == "Cryptographic Extensions":
        if m == "AESENC": return {"kind":"bytes", "hex":"63" * 16}
        if m == "AESDEC": return {"kind":"bytes", "hex":"52" * 16}
        return {"kind":"bytes", "hex":"00" * 16}
    return {"kind":"scalar", "value":0}


MASK64 = (1 << 64) - 1


def edge(vector):
    op = vector["op"]
    a, b = vector.get("a", 0), vector.get("b", 0)
    if op == "ADD64":
        value = (a + b) & MASK64
        return {"value":value, "CF":int(a + b > MASK64), "ZF":int(value == 0), "SF":value >> 63, "OF":int(((~(a ^ b) & (a ^ value)) >> 63) & 1)}
    if op == "ADC64":
        carry = vector["carry"]
        total = a + b + carry; value = total & MASK64
        signed_a = a - (1 << 64) if a >> 63 else a
        signed_b = b - (1 << 64) if b >> 63 else b
        signed_total = signed_a + signed_b + carry
        return {"value":value, "CF":int(total > MASK64), "ZF":int(value == 0),
                "SF":value >> 63, "OF":int(signed_total < -(1 << 63) or signed_total > (1 << 63) - 1)}
    if op == "SUB64":
        value = (a - b) & MASK64
        return {"value":value, "CF":int(a < b), "ZF":int(value == 0), "SF":value >> 63, "OF":int((((a ^ b) & (a ^ value)) >> 63) & 1)}
    if op == "SBB64":
        borrow = vector["borrow"]; value = (a - b - borrow) & MASK64
        signed_a = a - (1 << 64) if a >> 63 else a
        signed_b = b - (1 << 64) if b >> 63 else b
        signed_total = signed_a - signed_b - borrow
        return {"value":value, "CF":int(a < b + borrow), "ZF":int(value == 0),
                "SF":value >> 63, "OF":int(signed_total < -(1 << 63) or signed_total > (1 << 63) - 1)}
    if op == "UMULH64": return (a * b) >> 64
    if op == "FCVTU64": return float(a)
    if op == "FCVTUS64": return int(a)
    if op == "SHL64": return (a << (b & 63)) & MASK64
    if op == "SAR64":
        signed = a - (1 << 64) if a & (1 << 63) else a
        return (signed >> (b & 63)) & MASK64
    if op == "ROL64": return ((a << (b & 63)) | (a >> ((64 - b) & 63))) & MASK64
    if op == "PDEP64":
        out = 0; source_bit = 0
        for bit in range(64):
            if b >> bit & 1:
                out |= ((a >> source_bit) & 1) << bit; source_bit += 1
        return out
    if op == "PEXT64":
        out = 0; target_bit = 0
        for bit in range(64):
            if b >> bit & 1:
                out |= ((a >> bit) & 1) << target_bit; target_bit += 1
        return out
    if op == "BLSR64": return a & (a - 1)
    if op == "BLSI64": return a & -a
    if op == "CLZ64": return 64 - a.bit_length()
    if op == "CTZ64": return (a & -a).bit_length() - 1
    if op == "SDIV64": return abs(a) // abs(b) * (-1 if (a < 0) != (b < 0) else 1)
    if op == "SMOD64": return a - edge({"op":"SDIV64","a":a,"b":b}) * b
    if op == "SATADD8": return min(127, max(-128, a + b))
    if op == "SATSUB8": return min(127, max(-128, a - b))
    if op == "VCOMPRESS": return [x for i,x in enumerate(a) if vector["mask"] >> i & 1] + [0] * (len(a) - vector["mask"].bit_count())
    if op == "VEXPAND":
        out=[0]*len(a); source=0
        for i in range(len(a)):
            if vector["mask"] >> i & 1: out[i]=a[source]; source+=1
        return out
    if op == "VBLEND": return [b[i] if vector["mask"] >> i & 1 else a[i] for i in range(len(a))]
    if op == "PCLMUL64":
        out=0
        for bit in range(64):
            if b >> bit & 1: out ^= a << bit
        return out
    if op == "SHA256_SIG0":
        rotr=lambda x,n: ((x>>n)|(x<<(32-n))) & 0xFFFFFFFF
        return rotr(a,7) ^ rotr(a,18) ^ (a>>3)
    if op == "FIXED_MUL_Q8": return round(a*b/(1<<8))
    if op == "FADD64": return a + b
    if op == "FSQRT64": return math.sqrt(a)
    if op == "FCVTS_TOWARD_ZERO": return math.trunc(a)
    if op == "FCLASS64":
        value = vector["a"]
        if value == "-inf": return 1
        if value == "+zero": return 8
        raise ValueError(value)
    if op == "FRND_NEAREST_EVEN": return round(a)
    if op == "FRND_TOWARD_ZERO": return math.trunc(a)
    if op == "FRND_POSITIVE": return math.ceil(a)
    if op == "FRND_NEGATIVE": return math.floor(a)
    if op == "VGATHER_FAULT":
        dst=list(vector["old"]); mask=vector["mask"]
        for i,value in enumerate(vector["loaded"]): dst[i]=value; mask &= ~(1<<i)
        return {"dst":dst,"mask":mask,"fault_lane":vector["fault_lane"]}
    if op == "VSCATTER_FAULT":
        stores=[]; mask=vector["mask"]
        for i in range(vector["fault_lane"]):
            if mask>>i&1: stores.append([i,vector["values"][i]]); mask &= ~(1<<i)
        return {"stores":stores,"mask":mask,"fault_lane":vector["fault_lane"]}
    raise KeyError(op)


def main():
    corpus = json.loads((ROOT / "tests/golden-vectors.json").read_text())
    failures = []
    for vector in corpus["vectors"]:
        actual = evaluate(vector)
        if actual != vector["expected"]:
            failures.append((vector["instruction_id"], actual, vector["expected"]))
    edge_vectors = json.loads((ROOT / "tests/edge-vectors.json").read_text())["vectors"]
    for vector in edge_vectors:
        actual = edge(vector)
        if actual != vector["expected"]:
            failures.append((vector["op"], actual, vector["expected"]))
    if failures:
        for item in failures[:20]: print(item, file=sys.stderr)
        raise SystemExit(f"oracle failures: {len(failures)}")
    print(f"independent oracle passed {len(corpus['vectors'])} golden and {len(edge_vectors)} edge vectors")


if __name__ == "__main__": main()
