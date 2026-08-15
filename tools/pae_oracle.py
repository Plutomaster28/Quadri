#!/usr/bin/env python3
"""Independent executable checks for the SeaBird PAE32 walk contract."""

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def walk(vector, physical_bits):
    entries = vector["entries"]
    access = vector["access"]
    user = vector["cpl"] == 3
    readable = writable = user_ok = True
    accessed = []
    for level, entry in enumerate(entries):
        if not entry.get("P", 0):
            return {"fault": "NOT_PRESENT"}
        if entry.get("PFN", 0) >= 1 << (physical_bits - 12):
            return {"fault": "PHYSICAL_WIDTH"}
        readable &= bool(entry.get("R", 0))
        writable &= bool(entry.get("W", 0))
        user_ok &= bool(entry.get("U", 0))
        accessed.append(level)
        if entry.get("PS", 0):
            if level != 1 or entry["PFN"] & 0x1FF:
                return {"fault": "LARGE_PAGE"}
            break
    leaf = entries[len(accessed) - 1]
    if user and not user_ok:
        return {"fault": "USER"}
    if access == "read" and not readable:
        return {"fault": "READ"}
    if access == "write" and not writable:
        return {"fault": "WRITE"}
    if access == "execute" and leaf.get("XD", 0):
        return {"fault": "EXECUTE"}
    if leaf.get("PS", 0):
        pa = (leaf["PFN"] << 12) | (vector["va"] & 0x1FFFFF)
    else:
        pa = (leaf["PFN"] << 12) | (vector["va"] & 0xFFF)
    return {"pa": pa, "set_accessed": accessed,
            "set_dirty": [len(accessed) - 1] if access == "write" else []}


def main():
    corpus = json.loads((ROOT / "tests/pae-vectors.json").read_text())
    failures = []
    for vector in corpus["vectors"]:
        actual = walk(vector, corpus["physical_bits"])
        if actual != vector["expected"]:
            failures.append(f"{vector['name']}: {actual!r} != {vector['expected']!r}")
    if failures:
        raise SystemExit("PAE oracle failed:\n" + "\n".join(failures))
    print(f"PAE oracle passed {len(corpus['vectors'])} translation vectors")


if __name__ == "__main__":
    main()
