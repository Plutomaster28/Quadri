#!/usr/bin/env python3
"""Build SeaBird's machine-readable ISA database and generated references.

The v2.1 LaTeX allocation tables are imported once as the allocation ledger. Semantic
templates deliberately promote only instructions whose behavior can be stated without
inventing hidden state. Re-running this tool is deterministic.
"""

from __future__ import annotations

import hashlib
import json
import re
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "main (3).tex"
SPEC_DIR = ROOT / "spec"
DOCS_DIR = ROOT / "docs"
GEN_DIR = ROOT / "generated"
ALL_MODES = ["Clownfish", "Tetra", "Dragonet", "Droplet"]

SECTION_RE = re.compile(r"\\(?:sub)?section\{([^}]*)\}")
TOP_SECTION_RE = re.compile(r"^\\section\{([^}]*)\}")
ROW_RE = re.compile(r"^(.+?)\s*&\s*(.+?)\s*&\s*(.+?)\s*&\s*(.+?)\s*\\\\\s*(?:\\bottomrule)?$")
HEX_RE = re.compile(r"0x([0-9A-Fa-f]{2})")

IMPORT_TOP_SECTIONS = {
    "Instruction Encoding & Micro-Op Mapping", "Advanced Vector Extensions",
    "Cryptographic Extensions",
    "Digital Signal Processing (DSP) Extensions",
    "Architectural System Extensions",
}

ALIASES = {"LOAD": "LD", "STORE": "ST", "JZ": "JE", "JNZ": "JNE", "JMPR": "BRR"}

# v2 allocations that require hidden state, unspecified formats, or duplicate prefix
# machinery are retired in v3. Their byte values remain reserved and cannot be reused.
RETIRED_ALLOCATIONS = {
    "LOCK", "LOCKNOP", "XRESUME", "MOVM", "GCM_ENC_BLOCK", "GCM_DEC_BLOCK",
    "RNG_SEED", "AEAD_INIT", "AEAD_FINAL", "AESGCM_ENC", "AESGCM_DEC",
    "FIR_STEP", "IIR_STEP", "CONV_STEP", "FFT_STAGE", "VCOMPAND", "VMATRIX_MUL",
    "VCONVERT", "VMASKMOV", "MLAUNCH", "AESKEYGEN_ASSIST", "SHA256_RNDS2",
    "CTR_XOR", "SM4_ENC", "SM4_DEC", "CLMUL_RED", "RNG_GET",
}

PRIVILEGED = {
    "HLT", "RESET", "WRCR", "SYSRET", "IRET", "CLI", "STI", "SLEEP",
    "INVIC", "INVDC", "INVTLB", "INVTLBASID", "INVTLBALL", "SENDIPI", "EOI",
    "VMENTER", "VMRESUME", "VMREAD", "VMWRITE", "WRSS", "MLAUNCH", "LOADCTX",
    "SETMODE", "RNG_SEED",
}
HOST_ONLY = {"VMENTER", "VMRESUME", "VMREAD", "VMWRITE"}

FEATURE_BY_CATEGORY = {
    "Advanced Vector Extensions": "AVX",
    "Cryptographic Extensions": "CRYPTO",
    "Digital Signal Processing (DSP) Extensions": "DSP",
    "Architectural System Extensions": "SYSX",
    "Transactional Memory": "TXN",
    "SIMD / Vector": "SIMD",
    "Floating Point": "FP",
    "Atomic & Synchronization": "ATOMICS",
}

FLAG_NONE = {"CF": "unchanged", "PF": "unchanged", "AF": "unchanged", "ZF": "unchanged", "SF": "unchanged", "OF": "unchanged"}
FLAG_ARITH = {"CF": "defined", "PF": "defined", "AF": "defined", "ZF": "defined", "SF": "defined", "OF": "defined"}
FLAG_LOGIC = {"CF": "cleared", "PF": "defined", "AF": "undefined", "ZF": "defined", "SF": "defined", "OF": "cleared"}

PERFORMANCE_MARKERS = {
    "escape": 0xFD,
    "ordering": "FD marker-id precedes the optional FE primary prefix and the instruction opcode; exactly zero or one marker is permitted per instruction.",
    "unknown_id_behavior": "INVALID_OP before instruction decode or any architectural modification.",
    "markers": [
        {"id": 1, "name": "assume", "applicability": "Memory-accessing instructions.", "semantics": "The compiler expects ordinary completion. All standard faults, ordering, and recovery behavior remain unchanged."},
        {"id": 2, "name": "likely", "applicability": "Conditional control-flow instructions.", "semantics": "The taken edge is expected to be hot."},
        {"id": 3, "name": "unlikely", "applicability": "Conditional control-flow instructions.", "semantics": "The taken edge is expected to be cold."},
        {"id": 4, "name": "stream", "applicability": "Memory-reading or memory-writing instructions.", "semantics": "The accessed cache lines are expected to have little near-term reuse."},
        {"id": 5, "name": "prefetch", "applicability": "Memory-reading instructions.", "semantics": "The access is expected soon enough that an implementation may initiate translation or cache activity early; the instruction itself still executes normally."},
        {"id": 6, "name": "temporary", "applicability": "Instructions that produce a register result.", "semantics": "The produced value is expected to have a short live range."},
        {"id": 7, "name": "persistent", "applicability": "Instructions that produce a register result.", "semantics": "The produced value is expected to have a long live range."},
        {"id": 8, "name": "independent", "applicability": "Non-serializing instructions.", "semantics": "Alias analysis found no dependency on adjacent independently marked instructions; implementations must preserve architectural dependencies regardless."},
        {"id": 9, "name": "reuse", "applicability": "CALL and CALLA.", "semantics": "The logical call and window transition are unchanged; an implementation may reuse resident physical backing when it can prove the architectural window remains isolated."},
        {"id": 10, "name": "leaf", "applicability": "CALL and CALLA.", "semantics": "Software expects the destination not to perform a windowed call. The logical window transition is unchanged and the hint may be ignored."},
    ],
}


def clean_tex(value: str) -> str:
    value = value.replace("\\#", "#").replace("\\_", "_").replace("\\&", "&")
    value = value.replace("$", "").replace("\\texttt{", "").replace("}", "")
    value = value.replace("\\rightarrow", "->").replace("\\bottomrule", "")
    return re.sub(r"\s+", " ", value).strip()


def mnemonic_of(syntax: str) -> str:
    token = syntax.split()[0].strip().upper()
    token = token.split("(")[0]
    return token


def canonical_category(section: str) -> str:
    section = re.sub(r"\s*\([^)]*instructions?\)", "", section, flags=re.I)
    section = re.sub(r"\s*\(\d+ base \+ \d+ FPX instructions\)", "", section)
    return clean_tex(section)


def parse_allocations() -> list[dict]:
    current = "Uncategorized"
    active = False
    rows: list[dict] = []
    for line_no, raw in enumerate(SOURCE.read_text(encoding="utf-8").splitlines(), 1):
        top_match = TOP_SECTION_RE.match(raw)
        if top_match:
            active = canonical_category(top_match.group(1)) in IMPORT_TOP_SECTIONS
        section_match = SECTION_RE.search(raw)
        if section_match:
            current = canonical_category(section_match.group(1))
        if not active:
            continue
        match = ROW_RE.match(raw.strip())
        if not match or "textbf" in raw or "midrule" in raw:
            continue
        syntax, opcode_cell, fmt, description = map(clean_tex, match.groups())
        if not HEX_RE.search(opcode_cell):
            continue
        mnemonic = mnemonic_of(syntax)
        # Architecture 3.2 window operations are authored below with their
        # complete state/fault contracts; the legacy table importer must not
        # create a second generic SYSX entry for the documentation rows.
        if mnemonic in {"WINNEW", "WINPREV", "WINRESERVE", "WINPIN", "WINRELEASE"}:
            continue
        opcode = [int(x, 16) for x in HEX_RE.findall(opcode_cell)]
        if current == "Architectural System Extensions" and len(opcode) == 1:
            opcode = [0xFF, 0x04, opcode[0]]
        map_name = "base" if len(opcode) == 1 else {
            0x01: "AVX", 0x02: "CRYPTO", 0x03: "DSP", 0x04: "SYSX", 0x05: "FPX"
        }.get(opcode[1] if len(opcode) > 1 else -1, "extended")
        rows.append({
            "line": line_no,
            "syntax": syntax,
            "mnemonic": mnemonic,
            "category": current,
            "opcode": opcode,
            "map": map_name,
            "format": fmt,
            "description": description.rstrip(". "),
        })
    return rows


def operand_names(syntax: str) -> list[str]:
    tail = syntax[len(syntax.split()[0]):].strip()
    if not tail:
        return []
    return [x.strip() for x in tail.split(",")]


def operand_binding_for(row: dict) -> str:
    if row["mnemonic"] in RETIRED_ALLOCATIONS:
        return "Reserved encoding; no operand bytes are decoded."
    if row["mnemonic"] == "RDCR":
        return "ModR/M.mod = 11; ModR/M.reg = 0; ModR/M.r/m = Rdst; OREX extends only Rdst; trailing u16le field = CRn; canonical length = 4 bytes, or 7 bytes with FE/OREX."
    if row["mnemonic"] == "WRCR":
        return "ModR/M.mod = 11; ModR/M.reg = 0; ModR/M.r/m = Rsrc; OREX extends only Rsrc; trailing u16le field = CRn; canonical length = 4 bytes, or 7 bytes with FE/OREX."
    if row["mnemonic"] == "CMPXCHG":
        return "ModR/M.reg = Rexpected; ModR/M.r/m = [addr]; XOP0 = Rdesired."
    if row["mnemonic"] == "XCHG" and row["category"] == "Atomic & Synchronization":
        return "ModR/M.reg = Rvalue; ModR/M.r/m = [addr]."
    if row["mnemonic"] == "XCHG128":
        return "ModR/M.reg = Vvalue; ModR/M.r/m = [addr]."
    if row["mnemonic"] in {"OUT", "VMWRITE"}:
        selector, source = operand_names(row["syntax"])
        return f"ModR/M.mod = 11; ModR/M.reg = 0; ModR/M.r/m = {source}; trailing u16le field = {selector}."
    ops = operand_names(row["syntax"])
    if not ops:
        return "No ModR/M or XOP byte."
    is_mem = lambda value: "[" in value or "addr" in value.lower() or "vmcb" in value.lower() or "params" in value.lower()
    is_imm = lambda value: any(token in value.lower() for token in ("imm", "label", "rel", "mode", "field", "vector", "kind", "counter", "mask", "port", "shamt", "bits", "range", "format", "stage", " min", " max", " q")) and not value.lower().startswith(("r", "v", "k"))
    bindings = []
    consumed = 0
    if len(ops) == 1:
        if is_mem(ops[0]):
            bindings.append(f"ModR/M.r/m = {ops[0]}")
        elif is_imm(ops[0]) or ops[0].startswith("#"):
            bindings.append(f"immediate field = {ops[0]}")
        else:
            bindings.append(f"ModR/M.r/m = {ops[0]}; ModR/M.reg = 0")
        consumed = 1
    elif is_mem(ops[0]):
        bindings += [f"ModR/M.r/m = {ops[0]}", f"ModR/M.reg = {ops[1]}"]
        consumed = 2
    elif is_mem(ops[1]):
        bindings += [f"ModR/M.reg = {ops[0]}", f"ModR/M.r/m = {ops[1]}"]
        consumed = 2
    elif is_imm(ops[1]) or ops[1].startswith("#"):
        bindings += [f"ModR/M.r/m = {ops[0]}; ModR/M.reg = 0", f"immediate field = {ops[1]}"]
        consumed = 2
    else:
        bindings += [f"ModR/M.reg = {ops[0]}", f"ModR/M.r/m = {ops[1]}"]
        consumed = 2
    xop_index = 0
    for op in ops[consumed:]:
        if is_imm(op) or op.startswith("#"):
            bindings.append(f"immediate field = {op}")
        else:
            bindings.append(f"XOP{xop_index} = {op}")
            xop_index += 1
    return "; ".join(bindings) + "."


def simple_binary(mnemonic: str, op: str) -> list[str]:
    return ["width <- encoded operand width", f"result <- trunc_width(dst {op} src, width)", "dst <- result"]


def operation_for(row: dict) -> tuple[list[str], bool]:
    m = row["mnemonic"]
    ops = operand_names(row["syntax"])

    explicit = {
        "MOV": ["dst <- zero_extend(src, architectural_register_width)"],
        "MOVI": ["dst <- zero_extend(immediate, architectural_register_width)"],
        "MOVZX": ["dst <- zero_extend(src, destination_width)"],
        "MOVSX": ["dst <- sign_extend(src, destination_width)"],
        "MOVHI": ["dst.high_half <- src.low_half", "dst.low_half <- 0"],
        "MOVLO": ["dst <- zero_extend(src.low_half, destination_width)"],
        "MOVSWP": ["dst <- reverse_bytes(src, operand_width)"],
        "LD": ["address <- effective_address(mem)", "dst <- zero_extend(memory[address, operand_bytes], architectural_register_width)"],
        "ST": ["address <- effective_address(mem)", "memory[address, operand_bytes] <- low_bits(src, operand_width)"],
        "LDI": ["dst <- zero_extend(memory[zero_extend(immediate_address), operand_bytes], architectural_register_width)"],
        "LDB": ["dst <- zero_extend(memory[effective_address(mem), 1], architectural_register_width)"],
        "LDH": ["dst <- zero_extend(memory[effective_address(mem), 2], architectural_register_width)"],
        "LDW": ["dst <- zero_extend(memory[effective_address(mem), 4], architectural_register_width)"],
        "LDQ": ["dst <- zero_extend(memory[effective_address(mem), 8], architectural_register_width)"],
        "STB": ["memory[effective_address(mem), 1] <- low_bits(src, 8)"],
        "STH": ["memory[effective_address(mem), 2] <- low_bits(src, 16)"],
        "STW": ["memory[effective_address(mem), 4] <- low_bits(src, 32)"],
        "STQ": ["memory[effective_address(mem), 8] <- low_bits(src, 64)"],
        "LEA": ["dst <- effective_address(mem); no memory access is performed"],
        "LEAS": ["dst <- base + index * scale + sign_extend(displacement)"],
        "NEG": ["dst <- trunc_width(0 - dst, width)"],
        "INC": ["dst <- trunc_width(dst + 1, width)"],
        "DEC": ["dst <- trunc_width(dst - 1, width)"],
        "NOT": ["dst <- bitwise_not(dst, width)"],
        "CMP": ["result <- trunc_width(a - b, width); discard result"],
        "CMPI": ["result <- trunc_width(a - sign_extend(imm), width); discard result"],
        "CMPS": ["result <- signed_compare(a, b, width); update FLAGS only"],
        "CMPU": ["result <- unsigned_compare(a, b, width); update FLAGS only"],
        "TST": ["result <- a & b; discard result"],
        "TSTI": ["result <- a & zero_extend(imm); discard result"],
        "JMP": ["IP <- next_IP + sign_extend(relative_displacement)"],
        "JMPA": ["IP <- zero_extend(absolute_target, address_width)"],
        "CALL": ["SP <- SP - pointer_bytes", "memory[SP, pointer_bytes] <- next_IP", "if CR4.WINDOW_ENABLE then advance one logical register window", "IP <- next_IP + sign_extend(relative_displacement)"],
        "CALLA": ["target <- absolute_target before any window transition", "SP <- SP - pointer_bytes", "memory[SP, pointer_bytes] <- next_IP", "if CR4.WINDOW_ENABLE then advance one logical register window", "IP <- target"],
        "RET": ["target <- memory[SP, pointer_bytes]", "if CR4.WINDOW_ENABLE then validate and restore the previous logical register window", "SP <- SP + pointer_bytes", "IP <- target"],
        "BRR": ["IP <- low_address_bits(reg)"] ,
        "JZR": ["if reg == 0 then IP <- next_IP + sign_extend(displacement) else IP <- next_IP"],
        "JNZR": ["if reg != 0 then IP <- next_IP + sign_extend(displacement) else IP <- next_IP"],
        "TRAP": ["raise software interrupt vector immediate after saving a trap-class return frame"],
        "YIELD": ["record scheduler hint; architectural state is otherwise unchanged"],
        "PUSH": ["SP <- SP - pointer_bytes", "memory[SP, pointer_bytes] <- low_bits(src, pointer_width)"],
        "POP": ["value <- memory[SP, pointer_bytes]", "SP <- SP + pointer_bytes", "dst <- zero_extend(value)"],
        "PUSHF": ["SP <- SP - pointer_bytes", "memory[SP, pointer_bytes] <- FLAGS"],
        "POPF": ["new_flags <- memory[SP, pointer_bytes]", "validate writable FLAGS bits for CPL", "FLAGS <- new_flags", "SP <- SP + pointer_bytes"],
        "LFENCE": ["complete all older loads before any younger load executes"],
        "SFENCE": ["make all older stores globally visible before any younger store"],
        "MFENCE": ["complete all older loads, stores, and device accesses before younger memory accesses"],
        "FENCE": ["execute MFENCE semantics"],
        "ISYNC": ["discard stale prefetched instructions", "serialize all subsequent instruction fetch"],
        "QUERY": ["(R0, R1, R2, R3) <- query_leaf(leaf, subleaf)"],
        "GETCPL": ["dst <- zero_extend(CPL)"],
        "RDTIME": ["dst <- low_operand_width_bits(TIMER)"],
        "RDTS": ["dst <- low_operand_width_bits(TIMER)"],
        "GETPID": ["dst <- zero_extend(PID)"],
        "GETTID": ["dst <- zero_extend(TID)"],
        "ENDBR": ["architectural state unchanged; instruction is a valid IBT landing marker"],
    }
    if m in explicit:
        return explicit[m], True

    binary = {"ADD": "+", "ADDI": "+", "SUB": "-", "SUBI": "-", "MUL": "*", "MULI": "*", "AND": "&", "OR": "|", "XOR": "^", "NAND": "NAND", "NOR": "NOR", "XNOR": "XNOR"}
    if m in binary:
        src = "sign_extend(immediate)" if m.endswith("I") else "src"
        op = binary[m]
        return ["width <- encoded operand width", f"result <- trunc_width(dst {op} {src}, width)", "dst <- result"], True

    scalar_ops = {
        "DIV": ["if src == 0: raise DIV_ZERO", "dst <- signed(dst) / signed(src), rounded toward zero"],
        "DIVI": ["divisor <- sign_extend(immediate)", "if divisor == 0: raise DIV_ZERO", "dst <- signed(dst) / divisor, rounded toward zero"],
        "UDIV": ["if src == 0: raise DIV_ZERO", "dst <- unsigned(dst) / unsigned(src), rounded down"],
        "MOD": ["if src == 0: raise DIV_ZERO", "dst <- signed(dst) remainder signed(src); remainder has dividend sign"],
        "MODI": ["divisor <- sign_extend(immediate)", "if divisor == 0: raise DIV_ZERO", "dst <- signed(dst) remainder divisor"],
        "MULH": ["product <- signed(dst) * signed(src) at twice operand width", "dst <- high_width_bits(product)"],
        "UMUL": ["dst <- low_width_bits(unsigned(dst) * unsigned(src))"],
        "ADDS": ["dst <- clamp_signed(signed(dst) + signed(src), width)"],
        "ADDU": ["dst <- min(unsigned(dst) + unsigned(src), unsigned_max(width))"],
        "SUBS": ["dst <- clamp_signed(signed(dst) - signed(src), width)"],
        "SUBU": ["dst <- max(unsigned(dst) - unsigned(src), 0)"],
        "ABS": ["dst <- absolute_value(signed(src)); signed_min remains signed_min and sets OF"],
        "CLZ": ["dst <- count of consecutive zero bits from src[width-1] toward bit 0; zero input returns width"],
        "CTZ": ["dst <- count of consecutive zero bits from src[0] toward bit width-1; zero input returns width"],
        "POPC": ["dst <- number of one bits in low width bits of src"],
        "MAX": ["dst <- dst if signed(dst) >= signed(src) else src"],
        "MIN": ["dst <- dst if signed(dst) <= signed(src) else src"],
        "SLT": ["dst <- 1 if signed(dst) < signed(src) else 0"],
        "SGT": ["dst <- 1 if signed(dst) > signed(src) else 0"],
    }
    if m in scalar_ops:
        return scalar_ops[m], True

    bit_ops = {
        "SHL": ["count <- src mod width", "dst <- trunc_width(dst << count)"],
        "SHR": ["count <- src mod width", "dst <- unsigned(dst) >> count"],
        "SAR": ["count <- src mod width", "dst <- signed(dst) >> count with sign fill"],
        "ROL": ["count <- src mod width", "dst <- rotate_left(dst, count, width)"],
        "ROR": ["count <- src mod width", "dst <- rotate_right(dst, count, width)"],
        "BSET": ["index <- bit mod width", "dst[index] <- 1"],
        "BCLR": ["index <- bit mod width", "dst[index] <- 0"],
        "BTOG": ["index <- bit mod width", "dst[index] <- not dst[index]"],
        "BTST": ["index <- bit mod width", "ZF <- not dst[index]; all other FLAGS unchanged"],
        "MASK": ["dst <- src & zero_extend(mask, width)"],
        "EXT": ["validate start + length <= width", "dst <- zero_extend(src[start +: length])"],
        "PDEP": ["dst <- deposit low source bits into set-bit positions of mask in increasing bit order"],
        "PEXT": ["dst <- pack source bits selected by mask into low bits in increasing bit order"],
        "LZCNT": ["dst <- count_leading_zeros(src, width); zero input returns width"],
        "TZCNT": ["dst <- count_trailing_zeros(src, width); zero input returns width"],
        "POPCNT": ["dst <- population_count(src, width)"],
        "BEXTR": ["start <- imm[7:0]; length <- imm[15:8]", "dst <- zero_extend(src[start +: min(length, width-start)])"],
        "BINSERT": ["start <- imm[7:0]; length <- imm[15:8]", "dst[start +: length] <- low_bits(src, length)"],
        "BLSI": ["dst <- src & (0 - src)"],
        "BLSMSK": ["dst <- src ^ (src - 1)"],
        "BLSR": ["dst <- src & (src - 1)"],
        "RORX": ["dst <- rotate_right(src, immediate mod width, width); FLAGS unchanged"],
        "SHLX": ["dst <- trunc_width(src << (immediate mod width)); FLAGS unchanged"],
        "SHRX": ["dst <- unsigned(src) >> (immediate mod width); FLAGS unchanged"],
        "ANDN": ["dst <- bitwise_not(dst) & src"],
        "BZHI": ["index <- min(unsigned(src), width)", "dst <- dst & ((1 << index) - 1)"],
        "TZCNTV": ["dst <- count_trailing_zeros(src, width); zero input returns width"],
    }
    if m in bit_ops:
        return bit_ops[m], True

    transfer_ops = {
        "LDP": ["address <- effective_address(mem)", "validate both accesses before writeback", "dst1 <- memory[address, operand_bytes]", "dst2 <- memory[address + operand_bytes, operand_bytes]"],
        "STP": ["address <- effective_address(mem)", "validate both stores before either becomes visible", "memory[address] <- src1", "memory[address + operand_bytes] <- src2"],
        "XCHG": ["atomically exchange the two register operands, or perform one acquire+release read-modify-write when either operand is memory"],
        "PREFETCH": ["request that the cache line containing effective_address(mem) be fetched; faults are suppressed and state is unchanged"],
        "FLUSH": ["write back dirty data for the addressed cache line to the point of coherency and invalidate the local copy"],
        "INVIC": ["invalidate the local instruction-cache line containing the address; requires subsequent ISYNC before execution"],
        "INVDC": ["invalidate a clean local data-cache line; raise GPF if dirty data would be discarded"],
        "LDX": ["old_address <- base", "dst <- memory[old_address, operand_bytes]", "base <- old_address + operand_bytes after successful load"],
        "STX": ["old_address <- base", "memory[old_address, operand_bytes] <- src", "base <- old_address + operand_bytes after successful store"],
        "LDN": ["new_address <- base - operand_bytes", "dst <- memory[new_address, operand_bytes]", "base <- new_address after successful load"],
        "STN": ["new_address <- base - operand_bytes", "memory[new_address, operand_bytes] <- src", "base <- new_address after successful store"],
        "CPYB": ["copy len bytes from src to dst as if by increasing-address temporary-buffer semantics", "on fault, completed-byte count is written to R0 and remaining addresses to R1/R2"],
        "CPYW": ["copy len operand-width words from src to dst with temporary-buffer overlap semantics", "on fault, completed-word count is written to R0"],
        "MEMFILL": ["for i in 0 .. len-1: memory[dst + i * operand_bytes] <- low_operand_bits(value)", "on fault, completed-element count is written to R0"],
    }
    if m in transfer_ops:
        return transfer_ops[m], True

    condition = {
        "JE": "ZF == 1", "JNE": "ZF == 0", "JG": "ZF == 0 and SF == OF",
        "JGE": "SF == OF", "JL": "SF != OF", "JLE": "ZF == 1 or SF != OF",
        "JC": "CF == 1", "JNC": "CF == 0", "JO": "OF == 1", "JNO": "OF == 0",
        "JS": "SF == 1", "JNS": "SF == 0",
    }
    if m in condition:
        return [f"if {condition[m]} then IP <- next_IP + sign_extend(displacement) else IP <- next_IP"], True

    atomic_ops = {
        "CMPXCHG": ["address <- effective_address(mem)", "old <- atomic_load(memory[address, operand_bytes])", "if old == Rexpected: atomic_store(memory[address, operand_bytes], Rdesired); ZF <- 1", "else: Rexpected <- old; ZF <- 0"],
        "ATADD": ["old <- atomic_fetch_add(memory, src)", "dst <- old"],
        "ATSUB": ["old <- atomic_fetch_sub(memory, src)", "dst <- old"],
        "ATAND": ["old <- atomic_fetch_and(memory, src)", "dst <- old"],
        "ATOR": ["old <- atomic_fetch_or(memory, src)", "dst <- old"],
        "ATXOR": ["old <- atomic_fetch_xor(memory, src)", "dst <- old"],
        "LL": ["dst <- atomic_load(memory)", "reservation <- (physical_address, operand_bytes, coherence_version)"],
        "SC": ["if reservation still matches memory: atomic_store(memory, src); dst <- 1", "else: memory unchanged; dst <- 0", "clear reservation"],
        "XCHG128": ["require 16-byte alignment and ATOMIC128 feature", "atomically exchange 128-bit register value and memory operand"],
    }
    if m in atomic_ops:
        return atomic_ops[m], True

    transaction_ops = {
        "XBEGIN": ["checkpoint architectural state", "set transaction fallback to next_IP + sign_extend(displacement)", "enter transactional execution"],
        "XBEGINA": ["checkpoint architectural state", "set transaction fallback to absolute target", "enter transactional execution"],
        "XEND": ["if not in transaction: raise INVALID_OP", "attempt atomic commit; on failure abort to fallback"],
        "XABORT": ["set explicit-abort reason and low eight-bit user code", "restore checkpoint and branch to fallback"],
        "XTEST": ["dst <- 1 if transaction active else 0"],
        "XSTATUS": ["dst <- last transaction status bitfield; reading clears no bits"],
    }
    if m in transaction_ops:
        return transaction_ops[m], True

    system_ops = {
        "HLT": ["enter halted state until RESET, NMI, INIT, or an enabled interrupt"],
        "RESET": ["discard current context and enter the architecturally defined reset state"],
        "RDCR": ["validate system-register number and read permission", "dst <- zero_extend(system_register[CRn])"],
        "WRCR": ["validate register, value, privilege, and reserved bits", "commit system_register[CRn] <- src as one serializing operation"],
        "SYSCALL": ["create standard syscall frame on KSP0", "CPL <- 0; FLAGS <- FLAGS & not SYSMASK", "IP <- SYSENTRY"],
        "SYSRET": ["validate complete syscall frame", "atomically restore user SP, IP, FLAGS, and CPL"],
        "IRET": ["validate complete interrupt frame", "atomically restore SP, IP, FLAGS, and CPL"],
        "CLI": ["IF <- 0"], "STI": ["IF <- 1; maskable interrupts become eligible after the next instruction boundary"],
        "WFI": ["wait until an eligible interrupt or implementation event; architectural state is unchanged"],
        "SLEEP": ["request platform idle state immediate; return on interrupt or platform wake event"],
        "IN": ["validate IOPL or task I/O permission", "dst <- zero_extend(io_space[port, operand_bytes])"],
        "OUT": ["validate IOPL or task I/O permission", "io_space[port, operand_bytes] <- low_bits(src)"],
        "XSAVE": ["validate 64-byte aligned save area and component mask", "write requested enabled components at QUERY-reported offsets"],
        "XRSTOR": ["validate the entire requested image before state change", "atomically restore requested enabled components"],
        "INVTLB": ["invalidate local translation matching (va, asid) after older page-table stores are visible"],
        "INVTLBASID": ["invalidate all local non-global translations matching asid"],
        "INVTLBALL": ["invalidate every local translation including global entries"],
        "SENDIPI": ["publish an interrupt-controller message of kind to target with vector"],
        "EOI": ["remove vector from the local in-service set and permit lower-priority delivery"],
        "VMENTER": ["validate complete VMCB and stage-2 root", "save host state and atomically load guest state"],
        "VMRESUME": ["validate resumable VMCB", "save host state and atomically reload saved guest state"],
        "VMREAD": ["dst <- selected readable VMCB field"],
        "VMWRITE": ["validate field value and write selected writable VMCB field"],
        "WRSS": ["validate shadow-stack mapping", "memory[effective_address(mem), pointer_bytes] <- src using shadow-stack write permission"],
        "RDPMC": ["validate PMU enable and counter index", "dst <- performance_counter[counter]"],
        "RNGGET": ["request operand-width random bits", "on success dst <- random_bits and CF <- 1; on temporary failure dst <- 0 and CF <- 0"],
        "RNG_GET": ["request operand-width random bits", "on success dst <- random_bits and CF <- 1; on temporary failure dst <- 0 and CF <- 0"],
        "SAVECTX": ["validate aligned standard context area", "save core architectural context using the QUERY-reported revision"],
        "LOADCTX": ["validate complete standard context image", "atomically load core context; execution resumes at restored IP"],
        "SETMODE": ["validate target mode, CR3, and required feature state", "atomically enter target operating mode and serialize fetch"],
    }
    if m in system_ops:
        return system_ops[m], True

    stack_ops = {
        "PUSHA": ["snapshot R0-R31 including pre-instruction SP", "validate one 32-slot stack range", "SP <- SP - 32 * pointer_bytes", "store snapshot in increasing register order"],
        "POPA": ["validate and load all 32 slots before writeback", "restore R0-R31 atomically; restored R7 supplies final SP"],
        "ENTER": ["frame_size <- zero_extend(immediate) rounded to stack alignment", "push old R6", "R6 <- SP", "SP <- SP - frame_size"],
        "LEAVE": ["SP <- R6", "R6 <- memory[SP, pointer_bytes]", "SP <- SP + pointer_bytes"],
        "PUSHQ": ["SP <- SP - 2 * pointer_bytes", "store the named consecutive register pair in increasing register order"],
        "POPQ": ["validate both stack slots", "load named register pair", "SP <- SP + 2 * pointer_bytes"],
    }
    if m in stack_ops:
        return stack_ops[m], True

    base_vector = {
        "VADD": "a[i] + b[i]", "VSUB": "a[i] - b[i]", "VMUL": "a[i] * b[i]",
        "VDIV": "a[i] / b[i]", "VAND": "a[i] & b[i]", "VOR": "a[i] | b[i]",
        "VXOR": "a[i] ^ b[i]", "VSHL": "trunc_lane(a[i] << immediate)",
        "VSHR": "unsigned(a[i]) >> immediate", "VABS": "absolute_value(signed(a[i]))",
        "VMAX": "max_signed(a[i], b[i])", "VMIN": "min_signed(a[i], b[i])",
    }
    if m in base_vector:
        return ["for each active lane i:", f"    result[i] <- {base_vector[m]}", "apply encoded merge/zero mask policy", "commit destination vector atomically"], True
    if m == "VDUP":
        return ["for each destination lane i: result[i] <- source lane 0", "commit destination vector atomically"], True
    if m == "VLD":
        return ["validate every active lane address before destination writeback", "load active lanes in increasing lane order", "apply merge/zero mask policy and commit destination atomically"], True
    if m == "VST":
        return ["validate every active lane address before any store becomes visible", "store active lanes in increasing lane order as one precise instruction"], True

    fp_expr = {
        "FADD": "a + b", "FSUB": "a - b", "FMUL": "a * b", "FDIV": "a / b",
        "FSQRT": "square_root(a)", "FNEG": "copy_sign(a, not sign(a))", "FABS": "clear_sign(a)",
        "FMADD": "fused(a * b + c)", "FMSUB": "fused(a * b - c)",
        "FNMADD": "fused(-(a * b) + c)", "FNMSUB": "fused(-(a * b) - c)",
        "FMIN": "minimumNumber(a, b)", "FMAX": "maximumNumber(a, b)", "FRECIP": "1 / a",
        "FRSQRT": "1 / square_root(a)", "FRND": "round_integral(a, FPCR.rounding_mode)",
        "FRNDZ": "round_integral(a, toward_zero)", "FCHS": "copy_sign(a, not sign(a))",
    }
    if m in fp_expr:
        return [f"result <- correctly_rounded_IEEE754({fp_expr[m]}, destination_format, FPCR)", "update FPSR sticky exception bits", "if an unmasked exception exists: raise FPU_ERROR before writeback", "destination <- result; clear bits above scalar width"], True
    fp_special = {
        "FCMP": ["classify operands", "unordered: PF=1, ZF=1, CF=1; less: CF=1; equal: ZF=1; greater: all three clear", "clear OF, SF, AF"],
        "FCVTI": ["convert signed integer source to destination IEEE format using FPCR rounding"],
        "FCVTS": ["convert IEEE source to signed integer by truncation toward zero; invalid result raises or returns signed_min when masked"],
        "FCVT.S2D": ["convert binary32 source to exact binary64 destination"],
        "FCVT.D2S": ["convert binary64 source to binary32 using FPCR rounding"],
        "FCVTINT": ["convert between the source and encoded destination IEEE formats using FPCR rounding"],
        "FCLASS": ["dst <- classification bitmap: bit0 -inf, bit1 negative finite, bit2 -zero, bit3 +zero, bit4 positive finite, bit5 +inf, bit6 sNaN, bit7 qNaN"],
        "FTEST": ["dst <- FPSR exception bitmap; architectural FP state is otherwise unchanged"],
    }
    if m in fp_special:
        return fp_special[m], True

    advanced_vector = {
        "VFMADD": "result[i] <- correctly_rounded(d_old[i] + a[i] * b[i]) with one rounding",
        "VFMSUB": "result[i] <- correctly_rounded(d_old[i] - a[i] * b[i]) with one rounding",
        "VFNMADD": "result[i] <- correctly_rounded(d_old[i] - (a[i] * b[i])) with one rounding",
        "VFMADD_ROUND": "result[i] <- fused(d_old[i] + a[i] * b[i]) using encoded rounding mode",
        "VPERMUTE": "result[i] <- a[(imm >> (i * ceil_log2(lane_count))) & (lane_count - 1)]",
        "VSHUFFLE": "result byte i <- concat(a,b)[(imm >> (i * ceil_log2(2*vector_bytes))) & (2*vector_bytes - 1)]",
        "VBLEND": "result[i] <- b[i] when explicit mask bit i is one, otherwise a[i]",
        "VTEST": "result[i] <- all_ones when (d_old[i] & a[i]) != 0, otherwise zero",
        "VPMADD": "result[i] <- a[2*i] * b[2*i] + a[2*i+1] * b[2*i+1]",
        "VREDUCE_ADD": "result[0] <- left-to-right modular sum of all active a lanes; other lanes zero",
        "VREDUCE_MUL": "result[0] <- left-to-right modular product of all active a lanes; other lanes zero",
        "VCOMPARE_LT": "result[i] <- all_ones if signed(a[i]) < signed(b[i]) else zero",
        "VCOMPARE_GT": "result[i] <- all_ones if signed(a[i]) > signed(b[i]) else zero",
        "VINSERT": "result <- d_old; result[lane_index] <- low lane_width bits of a",
        "VEXTRACT": "result <- zero; result[0] <- a[lane_index]",
        "VALIGN": "result <- vector_bytes(concat(b,a), byte_offset, vector_bytes)",
        "VBSWAP": "result[i] <- reverse_bytes_within_lane(a[i])",
        "VPACK": "result <- concatenate(saturating_narrow(a lanes), saturating_narrow(b lanes))",
        "VUNPACK": "result even lanes <- zero_extend(a low half lanes); odd lanes <- zero_extend(b low half lanes)",
        "VPMUL": "result[i] <- low_lane_bits(unsigned(a[i]) * unsigned(b[i]))",
        "VPERM2": "result[i] <- concat(a,b)[(imm >> (i * ceil_log2(2*lane_count))) & (2*lane_count - 1)]",
        "VCOMPRESS": "pack lanes selected by mask into increasing low destination lanes; zero remaining lanes",
        "VEXPAND": "consume packed low source lanes into destination positions selected by mask; zero other lanes",
        "VROUND": "result[i] <- round_integral(a[i], encoded_rounding_mode)",
        "VRECIP_EST": "result[i] <- implementation estimate of 1/a[i] with relative error <= 2^-14 for normal finite inputs",
        "VRSQRT_EST": "result[i] <- implementation estimate of 1/sqrt(a[i]) with relative error <= 2^-14 for positive normal inputs",
        "VFMADD_SUB": "result[i] <- fused(d_old[i] + a[i]*b[i]) for even i and fused(d_old[i] - a[i]*b[i]) for odd i",
        "VZEROUPPER": "for every vector register: clear bits 128 through enabled_vector_width-1",
        "VZEROALL": "clear all enabled bits of V0-V31 and K0-K7",
        "VPMAX": "result[i] <- max_signed(a[i], b[i])",
        "VPMIN": "result[i] <- min_signed(a[i], b[i])",
        "VFPCLASS": "result[i] <- IEEE classification bitmap for a[i]",
        "VREDUCE_MAX": "result[0] <- maximum active a lane; other lanes zero",
        "VREDUCE_MIN": "result[0] <- minimum active a lane; other lanes zero",
        "VMULADDSUB": "result[i] <- low_lane_bits(a[i]*b[i] + d_old[i]) for even i and low_lane_bits(a[i]*b[i] - d_old[i]) for odd i",
    }
    if m in {"VZEROUPPER", "VZEROALL"}:
        return [advanced_vector[m]], True
    if m in advanced_vector:
        return [advanced_vector[m], "apply encoded merge/zero mask policy", "commit complete vector destination atomically"], True
    if m in {"VGATHER", "VGATHERQ"}:
        size = "8" if m.endswith("Q") else "lane_bytes"
        return [f"for each active lane i: address[i] <- base + signed(index[i]) * scale; element size <- {size}", "load active lanes in increasing order", "on fault commit completed lanes and clear their mask bits; leave later lanes and mask bits unchanged"], True
    if m in {"VSCATTER", "VSCATTERQ"}:
        size = "8" if m.endswith("Q") else "lane_bytes"
        return [f"for each active lane i: address[i] <- base + signed(index[i]) * scale; element size <- {size}", "store active lanes in increasing order", "on fault completed stores remain visible and completed mask bits are cleared"], True

    crypto_ops = {
        "AESENC": "dst <- MixColumns(ShiftRows(SubBytes(a))) XOR roundkey, using AES column-major byte order",
        "AESDEC": "dst <- InvMixColumns(InvShiftRows(InvSubBytes(a))) XOR roundkey",
        "AESIMC": "dst <- InvMixColumns(a)",
        "AESKEYGEN_ASSIST": "dst <- AES key-schedule RotWord/SubWord/Rcon transform selected by immediate",
        "PCLMULQDQ": "dst <- carryless_product(selected_64(a), selected_64(b)); result is 128 bits",
        "GHASH": "dst <- gf128_multiply(dst_old XOR a, H) modulo x^128+x^7+x^2+x+1, bit-reflected GCM order",
        "SHA1_MSG1": "dst <- SHA-1 message schedule first transform on four 32-bit lanes",
        "SHA1_MSG2": "dst <- SHA-1 message schedule second rotate/XOR transform on four 32-bit lanes",
        "SHA256_RNDS2": "dst <- two SHA-256 compression rounds using a and implicit packed state operands defined by the syntax",
        "SHA256_SIG0": "for each 32-bit lane x: dst <- rotr(x,7) XOR rotr(x,18) XOR (x>>3)",
        "SHA256_SIG1": "for each 32-bit lane x: dst <- rotr(x,17) XOR rotr(x,19) XOR (x>>10)",
        "RNG_GET": "request operand-width random bits; CF=1 and dst=data on success, CF=0 and dst=0 on temporary failure",
        "POLY_MUL": "dst <- carryless polynomial product of a and b truncated to destination vector width",
        "CTR_XOR": "dst <- a XOR AES_encrypt(key, counter); counter operand is not modified",
        "SM4_ENC": "dst <- one complete SM4 block encryption using the 128-bit key operand",
        "SM4_DEC": "dst <- one complete SM4 block decryption using reversed round-key order",
        "CLMUL_RED": "dst <- reduce carryless 256-bit source modulo the polynomial selected by immediate",
    }
    if m in crypto_ops:
        return [crypto_ops[m]], True

    dsp_ops = {
        "MAC32": "dst <- trunc_width(dst + signed32(a) * signed32(b))",
        "MAC64": "dst <- trunc_width(dst + signed64(a) * signed64(b))",
        "MACS": "dst <- saturate_signed(dst + signed(a) * signed(b), width)",
        "MSUB": "dst <- trunc_width(dst - a * b)",
        "SATSUB": "dst <- saturate_signed(a - b, width)",
        "SATADD": "dst <- saturate_signed(a + b, width)",
        "FIXED_MUL": "dst <- round_to_nearest_even((signed(a) * signed(b)) / 2^Q), saturated to width",
        "FIXED_ADD": "dst <- saturate_signed(a + b, width)",
        "CMPLX_MUL": "(dst_real,dst_imag) <- (a_real*b_real-a_imag*b_imag, a_real*b_imag+a_imag*b_real)",
        "BITREV": "dst low bits <- reverse_bit_order(a low 'bits' bits); higher bits zero",
        "PACK_SAT": "dst <- concatenate(saturating_narrow(a), saturating_narrow(b))",
        "UNPACK_EXP": "dst lanes <- sign_extend corresponding packed source lanes to twice lane width",
        "CLAMP": "dst <- min(max(signed(a), signed(min)), signed(max))",
        "ACCUMULATE": "dst <- trunc_width(dst + a); CF receives carry out",
        "DOTP": "dst <- left-to-right sum of signed products of corresponding active vector lanes",
        "SUMDOTP": "dst <- left-to-right sum of signed adjacent-lane products within a",
        "RSHIFT_ROUND": "dst <- round_to_nearest_even(signed(a) / 2^shamt)",
        "LSHIFT": "dst <- trunc_width(a << (shamt mod width))",
        "SLLV": "dst <- trunc_width(a << (b mod width))",
        "SRLV": "dst <- unsigned(a) >> (b mod width)",
        "SRAV": "dst <- signed(a) >> (b mod width) with sign fill",
        "RNDQ": "dst <- round_to_nearest_even(signed(a) / 2^q) * 2^q",
        "CLZ_FAST": "dst <- count_leading_zeros(a, width); architecturally identical to CLZ",
        "TZCNT_FAST": "dst <- count_trailing_zeros(a, width); architecturally identical to CTZ",
        "MAD32": "dst <- trunc_width(signed32(a) * signed32(b) + c)",
    }
    if m in dsp_ops:
        return [dsp_ops[m]], True

    if m.startswith("V") and row["category"] in {"SIMD / Vector", "Advanced Vector Extensions"}:
        return ["for each active destination lane i in increasing index order:", f"    lane_result[i] <- {m}_lane(source_operands, i, lane_width)", "apply encoded merge/zero mask policy", "commit the complete vector destination atomically"], False
    if row["category"] == "Floating Point":
        return [f"result <- IEEE_754_{m}(source_operands, FPCR)", "update FPSR sticky exceptions", "if an unmasked exception exists: raise FPU_ERROR before destination write", "destination <- result"], False
    if row["category"] in {"Cryptographic Extensions", "Digital Signal Processing (DSP) Extensions"}:
        return [f"reserved v2 allocation {m}; execution raises INVALID_OP"], False
    if row["category"] == "Atomic & Synchronization" or m.startswith("AT"):
        return ["require naturally aligned operand contained in one cache line", f"(old, new) <- atomic_{m}(memory_operand, register_operands)", "commit one indivisible coherent read-modify-write", "destination <- old when the syntax names a destination"], False
    if row["category"] == "Transactional Memory":
        return [f"perform transaction_control_{m}(operands); on abort restore checkpoint and branch to fallback"], False
    return [f"perform {m} according to the description: {row['description']}"], False


def flags_for(row: dict) -> dict:
    m = row["mnemonic"]
    if m in {"ADD", "ADDI", "SUB", "SUBI", "CMP", "CMPI", "CMPS", "CMPU"}:
        return dict(FLAG_ARITH)
    if m in {"AND", "OR", "XOR", "NAND", "NOR", "XNOR", "TST", "TSTI"}:
        return dict(FLAG_LOGIC)
    if m in {"INC", "DEC"}:
        out = dict(FLAG_ARITH); out["CF"] = "unchanged"; return out
    if m == "CMPXCHG":
        out = dict(FLAG_NONE); out["ZF"] = "defined"; return out
    if m in {"SHL", "SHR", "SAR", "ROL", "ROR"}:
        return {"CF": "defined", "PF": "defined for shifts; unchanged for rotates", "AF": "undefined", "ZF": "defined for shifts; unchanged for rotates", "SF": "defined for shifts; unchanged for rotates", "OF": "defined only for count 1"}
    return dict(FLAG_NONE)


def memory_order_for(row: dict) -> str:
    m = row["mnemonic"]
    if row["category"] == "Atomic & Synchronization" or m in {"XCHG", "CMPXCHG"}:
        return "Atomic read-modify-write with acquire+release ordering; .SC form is sequentially consistent."
    if m in {"LFENCE", "SFENCE", "MFENCE", "FENCE", "ISYNC"}:
        return "Ordering is the operation itself."
    if m == "PREFETCH":
        return "Non-binding cache hint; creates no memory-order edge and suppresses address, translation, and permission faults."
    if m in {"FLUSH", "INVIC", "INVDC"}:
        return "Cache-maintenance operation ordered after older accesses to the addressed line; use the required fence or ISYNC before relying on global visibility or refetch."
    if m in {"CPYB", "CPYW", "MEMFILL"}:
        return "Restartable sequence of ordinary TSO accesses in increasing element order; completed stores remain visible if a later element faults."
    if m in {"VGATHER", "VGATHERQ", "VSCATTER", "VSCATTERQ"}:
        return "Lane-ordered ordinary TSO accesses with the instruction's documented partial-progress state on fault."
    if m in {"CALL", "CALLA", "RET", "PUSH", "POP", "PUSHA", "POPA",
             "ENTER", "LEAVE", "PUSHF", "POPF", "PUSHQ", "POPQ"}:
        return "Ordinary TSO stack access using the active architectural stack pointer."
    if m in {"XSAVE", "XRSTOR", "VMENTER", "VMRESUME", "WRSS",
             "SAVECTX", "LOADCTX"}:
        return "Architectural multi-byte memory access with the validation and atomicity contract stated by the instruction."
    if any(x in row["format"] for x in ("M-type", "+ M")) or m.startswith(("LD", "ST", "VLD", "VST")):
        return "Ordinary TSO access unless the instruction entry states atomic or device ordering."
    return "No memory access."


def exceptions_for(row: dict) -> list[str]:
    m = row["mnemonic"]
    result = ["INVALID_OP for malformed encoding or unavailable feature"]
    if row["privilege"] != "user":
        result.append("GPF when current privilege is insufficient")
    memory = memory_order_for(row) != "No memory access." and m != "PREFETCH"
    if memory:
        result += ["GPF for non-canonical address", "ALIGN_CHECK when required alignment is violated", "PAGE_FAULT for translation or permission failure"]
    if m in {"DIV", "DIVI", "UDIV", "MOD", "MODI", "VDIV", "FDIV"}:
        result.append("DIV_ZERO or floating-point divide-by-zero before destination write")
    if row["category"] == "Floating Point" or m.startswith("VF"):
        result.append("FPU_ERROR or SIMD_ERROR for an unmasked IEEE exception")
    return result


def instruction_id(row: dict, ordinal: int) -> str:
    suffix = hashlib.sha1((row["syntax"] + str(row["opcode"])).encode()).hexdigest()[:6]
    return f"SB-{row['map']}-{row['mnemonic']}-{ordinal:03d}-{suffix}".upper()


def build_database(rows: list[dict]) -> dict:
    seen_syntax: Counter[str] = Counter()
    instructions = []
    for row in rows:
        seen_syntax[row["mnemonic"]] += 1
        row["privilege"] = "host" if row["mnemonic"] in HOST_ONLY else "kernel" if row["mnemonic"] in PRIVILEGED else "user"
        operation, fully_defined = operation_for(row)
        if row["mnemonic"] in RETIRED_ALLOCATIONS:
            status = "reserved"
            operation = ["reserved v2 allocation; execution raises INVALID_OP"]
        else:
            status = "normative" if fully_defined else "provisional"
        feature = FEATURE_BY_CATEGORY.get(row["category"], "BASE")
        if row["map"] not in {"base", "extended"}:
            feature = row["map"]
        inst = {
            "id": instruction_id(row, seen_syntax[row["mnemonic"]]),
            "mnemonic": row["mnemonic"],
            "syntax": row["syntax"],
            "category": row["category"],
            "encoding": {"map": row["map"], "opcode": row["opcode"], "format": row["format"], "operand_binding": operand_binding_for(row),
                         **({"system_register_field": {"width_bits": 16, "byte_order": "little", "required": True},
                              "canonical_length_bytes": {"without_orex": 4, "with_orex": 7}}
                            if row["mnemonic"] in {"RDCR", "WRCR"} else {})},
            "feature": feature,
            "privilege": row["privilege"],
            "modes": ALL_MODES,
            "description": row["description"],
            "operation": operation,
            "flags": flags_for(row),
            "memory_order": memory_order_for(row),
            "exceptions": exceptions_for(row),
            "status": status,
            "source_line": row["line"],
        }
        instructions.append(inst)
    scalar_fp_memory = [
        ("FLD", "FLD Fd, [addr]", 16,
         "ModR/M.reg = Fd; ModR/M.r/m = [addr].",
         "Load one scalar floating-point value without accessing adjacent bytes",
         ["read exactly operand_width bits from memory", "Fd.low <- loaded bit pattern", "clear destination bits above scalar width"],
         "Ordinary scalar load."),
        ("FST", "FST [addr], Fs", 17,
         "ModR/M.r/m = [addr]; ModR/M.reg = Fs.",
         "Store one scalar floating-point value without accessing adjacent bytes",
         ["write exactly operand_width low bits of Fs to memory as a little-endian scalar"],
         "Ordinary scalar store."),
    ]
    for mnemonic, syntax, subopcode, binding, description, operation, ordering in scalar_fp_memory:
        instructions.append({
            "id": f"SB-FPX-{mnemonic}-RC2-001", "mnemonic": mnemonic,
            "syntax": syntax, "category": "Floating Point",
            "encoding": {"map": "FPX", "opcode": [255, 5, subopcode],
                         "format": "M-type (scalar FP)",
                         "operand_binding": binding + " Operand-width prefix selects 16/32/64/128-bit scalar format."},
            "feature": "FPX", "privilege": "user", "modes": ALL_MODES,
            "description": description, "operation": operation,
            "flags": dict(FLAG_NONE), "memory_order": ordering,
            "exceptions": ["INVALID_OP for malformed encoding or unavailable feature",
                           "ALIGN_CHECK, PAGE_FAULT, or GPF before architectural modification"],
            "status": "normative", "source_line": 0,
        })
    extra_instructions = [
        {
            "id": "SB-BASE-ADC-001-V31", "mnemonic": "ADC",
            "syntax": "ADC Rdst, Rsrc", "category": "Arithmetic",
            "encoding": {"map": "base", "opcode": [0xD8], "format": "R-type",
                         "operand_binding": "ModR/M.reg = Rdst; ModR/M.r/m = Rsrc."},
            "feature": "BASE", "privilege": "user", "modes": ALL_MODES,
            "description": "Add source and carry flag to destination",
            "operation": ["carry_in <- CF", "sum <- unsigned(dst) + unsigned(src) + carry_in",
                          "dst <- low_width_bits(sum)", "update arithmetic FLAGS including carry_out"],
            "flags": dict(FLAG_ARITH), "memory_order": "No memory access.",
            "exceptions": ["INVALID_OP for malformed encoding or unavailable feature"],
            "status": "normative", "source_line": 0,
        },
        {
            "id": "SB-BASE-SBB-001-V31", "mnemonic": "SBB",
            "syntax": "SBB Rdst, Rsrc", "category": "Arithmetic",
            "encoding": {"map": "base", "opcode": [0xD9], "format": "R-type",
                         "operand_binding": "ModR/M.reg = Rdst; ModR/M.r/m = Rsrc."},
            "feature": "BASE", "privilege": "user", "modes": ALL_MODES,
            "description": "Subtract source and borrow flag from destination",
            "operation": ["borrow_in <- CF", "difference <- unsigned(dst) - unsigned(src) - borrow_in",
                          "dst <- low_width_bits(difference)", "update arithmetic FLAGS including borrow_out"],
            "flags": dict(FLAG_ARITH), "memory_order": "No memory access.",
            "exceptions": ["INVALID_OP for malformed encoding or unavailable feature"],
            "status": "normative", "source_line": 0,
        },
        {
            "id": "SB-BASE-UMULH-001-V31", "mnemonic": "UMULH",
            "syntax": "UMULH Rdst, Rsrc", "category": "Arithmetic",
            "encoding": {"map": "base", "opcode": [0xDA], "format": "R-type (unsigned high bits)",
                         "operand_binding": "ModR/M.reg = Rdst; ModR/M.r/m = Rsrc."},
            "feature": "BASE", "privilege": "user", "modes": ALL_MODES,
            "description": "Return the high half of an unsigned full-width product",
            "operation": ["product <- unsigned(dst) * unsigned(src) at twice operand width",
                          "dst <- high_width_bits(product)"],
            "flags": dict(FLAG_NONE), "memory_order": "No memory access.",
            "exceptions": ["INVALID_OP for malformed encoding or unavailable feature"],
            "status": "normative", "source_line": 0,
        },
        {
            "id": "SB-BASE-FCVTU-001-V31", "mnemonic": "FCVTU",
            "syntax": "FCVTU Fd, Ra", "category": "Floating Point",
            "encoding": {"map": "base", "opcode": [0xDB], "format": "R-type (unsigned conversion)",
                         "operand_binding": "ModR/M.reg = Fd; ModR/M.r/m = Ra; VectorCtl selects the scalar IEEE format."},
            "feature": "FP", "privilege": "user", "modes": ALL_MODES,
            "description": "Convert an unsigned integer to a scalar IEEE value",
            "operation": ["convert unsigned integer source to destination IEEE format using FPCR rounding",
                          "update FPSR; raise an unmasked FPU_ERROR before writeback"],
            "flags": dict(FLAG_NONE), "memory_order": "No memory access.",
            "exceptions": ["INVALID_OP for malformed encoding or unavailable feature",
                           "FPU_ERROR for an unmasked IEEE exception"],
            "status": "normative", "source_line": 0,
        },
        {
            "id": "SB-BASE-FCVTUS-001-V31", "mnemonic": "FCVTUS",
            "syntax": "FCVTUS Ra, Fs", "category": "Floating Point",
            "encoding": {"map": "base", "opcode": [0xDC], "format": "R-type (unsigned conversion)",
                         "operand_binding": "ModR/M.reg = Ra; ModR/M.r/m = Fs; VectorCtl selects the scalar IEEE format."},
            "feature": "FP", "privilege": "user", "modes": ALL_MODES,
            "description": "Convert a scalar IEEE value to an unsigned integer",
            "operation": ["convert IEEE source to unsigned integer by truncation toward zero",
                          "invalid result raises FPU_ERROR or returns zero when masked"],
            "flags": dict(FLAG_NONE), "memory_order": "No memory access.",
            "exceptions": ["INVALID_OP for malformed encoding or unavailable feature",
                           "FPU_ERROR for an unmasked IEEE exception"],
            "status": "normative", "source_line": 0,
        },
    ]
    for subopcode, mnemonic, relation in [
        (0x2B, "VCOMPARE_EQ", "a[i] == b[i]"),
        (0x2C, "VCOMPARE_NE", "a[i] != b[i]"),
        (0x2D, "VCOMPARE_ULT", "unsigned(a[i]) < unsigned(b[i])"),
        (0x2E, "VCOMPARE_UGT", "unsigned(a[i]) > unsigned(b[i])"),
    ]:
        extra_instructions.append({
            "id": f"SB-AVX-{mnemonic}-001-V31", "mnemonic": mnemonic,
            "syntax": f"{mnemonic} Vd, Va, Vb", "category": "Advanced Vector Extensions",
            "encoding": {"map": "AVX", "opcode": [0xFF, 0x01, subopcode], "format": "V-type",
                         "operand_binding": "ModR/M.reg = Vd; ModR/M.r/m = Va; XOP0 = Vb; VectorCtl is required."},
            "feature": "AVX", "privilege": "user", "modes": ALL_MODES,
            "description": f"Per-lane predicate for {relation}",
            "operation": [f"result[i] <- all_ones if {relation} else zero",
                          "apply encoded merge/zero mask policy", "commit complete vector destination atomically"],
            "flags": dict(FLAG_NONE), "memory_order": "No memory access.",
            "exceptions": ["INVALID_OP for malformed encoding or unavailable feature",
                           "SIMD_ERROR for an invalid vector-control combination"],
            "status": "normative", "source_line": 0,
        })
    extra_instructions.append({
        "id": "SB-AVX-VNOT-001-V31", "mnemonic": "VNOT",
        "syntax": "VNOT Vd, Va", "category": "Advanced Vector Extensions",
        "encoding": {"map": "AVX", "opcode": [0xFF, 0x01, 0x2F], "format": "V-type",
                     "operand_binding": "ModR/M.reg = Vd; ModR/M.r/m = Va; VectorCtl is required."},
        "feature": "AVX", "privilege": "user", "modes": ALL_MODES,
        "description": "Per-lane vector bitwise complement",
        "operation": ["result[i] <- bitwise_not(a[i])", "apply encoded merge/zero mask policy",
                      "commit complete vector destination atomically"],
        "flags": dict(FLAG_NONE), "memory_order": "No memory access.",
        "exceptions": ["INVALID_OP for malformed encoding or unavailable feature",
                       "SIMD_ERROR for an invalid vector-control combination"],
        "status": "normative", "source_line": 0,
    })
    for subopcode, mnemonic, description, operation, privilege in [
        (0x1B, "WINNEW", "Advance to a fresh logical register window without control transfer",
         ["validate window state and spill-area configuration", "create a child window; map caller R24-R31 to child R8-R15", "initialize child R16-R31 to zero", "make the child window current atomically"], "user"),
        (0x1C, "WINPREV", "Restore the previous logical register window without returning",
         ["require a previous logical window", "propagate child R8-R15 through the overlap to parent R24-R31", "make the parent window current atomically; restore it transparently if spilled"], "user"),
        (0x1D, "WINRESERVE", "Hint that another logical window will be needed soon",
         ["optionally prepare microarchitectural resident capacity", "do not spill, fault, or change architectural window state"], "user"),
        (0x1E, "WINPIN", "Request advisory retention of the current logical window",
         ["increment the current window retention hint count", "do not prevent a spill required for correctness or forward progress"], "user"),
        (0x1F, "WINRELEASE", "Release one advisory window-retention request",
         ["require a nonzero retention hint count", "decrement the current window retention hint count"], "user"),
    ]:
        may_spill = mnemonic in {"WINNEW", "WINPREV"}
        window_exceptions = [
            "INVALID_OP for malformed encoding or unavailable WINDOW feature",
            "GPF when CR4.WINDOW_ENABLE is clear",
        ]
        if mnemonic == "WINNEW":
            window_exceptions.append("GPF for invalid or exhausted WSP configuration")
        elif mnemonic == "WINPREV":
            window_exceptions.append("GPF for window underflow or invalid WSP configuration")
        elif mnemonic == "WINRELEASE":
            window_exceptions.append("GPF for an unbalanced release")
        if may_spill:
            window_exceptions.append("ALIGN_CHECK or PAGE_FAULT before architectural modification if a transparent spill/restore fails")
        extra_instructions.append({
            "id": f"SB-SYSX-{mnemonic}-001-V10", "mnemonic": mnemonic,
            "syntax": mnemonic, "category": "Architectural System Extensions",
            "encoding": {"map": "SYSX", "opcode": [0xFF, 0x04, subopcode],
                         "format": "X-type (no operands)",
                         "operand_binding": "No ModR/M, XOP, immediate, or displacement bytes."},
            "feature": "WINDOW", "privilege": privilege, "modes": ALL_MODES,
            "description": description, "operation": operation,
            "flags": dict(FLAG_NONE),
            "memory_order": ("May perform transparent window spill or restore memory accesses; those accesses are ordered as ordinary core-context loads/stores and complete before retirement."
                             if may_spill else "No memory access."),
            "exceptions": window_exceptions,
            "status": "normative", "source_line": 0,
        })
    instructions.extend(extra_instructions)
    for alias, canonical in ALIASES.items():
        instructions.append({
            "id": f"SB-ALIAS-{alias}", "mnemonic": alias, "syntax": alias,
            "category": "Aliases", "encoding": {"map": "alias", "opcode": [], "format": "assembler alias", "operand_binding": "Inherited from canonical instruction."},
            "feature": "BASE", "privilege": "user", "modes": ALL_MODES,
            "description": f"Assembler alias for {canonical}", "operation": [f"assemble as {canonical}"],
            "flags": dict(FLAG_NONE), "memory_order": "Inherited from canonical instruction.",
            "exceptions": ["Inherited from canonical instruction."], "status": "alias", "alias_of": canonical,
        })
    return {"schema_version": 2, "architecture_version": "3.2",
            "performance_markers": PERFORMANCE_MARKERS,
            "instructions": instructions}


def validate(db: dict) -> None:
    required = {"id", "mnemonic", "syntax", "category", "encoding", "feature", "privilege", "modes", "description", "operation", "flags", "memory_order", "exceptions", "status"}
    ids = set()
    encodings: dict[tuple[int, ...], str] = {}
    marker_ids = [x["id"] for x in db["performance_markers"]["markers"]]
    marker_names = [x["name"] for x in db["performance_markers"]["markers"]]
    if len(marker_ids) != len(set(marker_ids)) or len(marker_names) != len(set(marker_names)):
        raise ValueError("duplicate performance marker ID or name")
    if db["performance_markers"]["escape"] in {0xFE, 0xFF}:
        raise ValueError("performance marker escape collides with an existing prefix")
    for inst in db["instructions"]:
        missing = required - inst.keys()
        if missing:
            raise ValueError(f"{inst.get('mnemonic')}: missing {sorted(missing)}")
        if inst["id"] in ids:
            raise ValueError(f"duplicate id: {inst['id']}")
        ids.add(inst["id"])
        opcode = tuple(inst["encoding"]["opcode"])
        if opcode and inst["status"] != "alias":
            previous = encodings.get(opcode)
            if previous:
                raise ValueError(f"encoding collision {opcode}: {previous} and {inst['syntax']}")
            encodings[opcode] = inst["syntax"]
        if not inst["operation"] or not inst["exceptions"]:
            raise ValueError(f"incomplete entry: {inst['syntax']}")


def cpp_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def write_cpp(db: dict) -> None:
    entries = [x for x in db["instructions"] if x["encoding"]["opcode"]]
    lines = [
        "// Generated by tools/build_isa_spec.py. Do not edit.", "#pragma once", "#include <array>", "#include <cstdint>", "#include <string_view>", "", "namespace seabird::isa {",
        "struct OpcodeEntry { std::string_view mnemonic; std::string_view syntax; std::string_view operand_binding; std::array<std::uint8_t, 3> bytes; std::uint8_t length; std::uint8_t operand_count; bool normative; };",
        f"inline constexpr std::array<OpcodeEntry, {len(entries)}> kOpcodes{{{{",
    ]
    for inst in entries:
        op = inst["encoding"]["opcode"] + [0, 0, 0]
        count = len(operand_names(inst["syntax"]))
        lines.append(f'  {{"{cpp_string(inst["mnemonic"])}", "{cpp_string(inst["syntax"])}", "{cpp_string(inst["encoding"]["operand_binding"])}", {{{op[0]}, {op[1]}, {op[2]}}}, {len(inst["encoding"]["opcode"])}, {count}, {str(inst["status"] == "normative").lower()}}},')
    lines += ["}};", "}  // namespace seabird::isa", ""]
    (GEN_DIR / "seabird_opcodes.hpp").write_text("\n".join(lines), encoding="utf-8")


def tex_escape(value: str) -> str:
    replacements = [("\\", r"\textbackslash{}"), ("_", r"\_"), ("#", r"\#"), ("&", r"\&"), ("%", r"\%"), ("^", r"\textasciicircum{}"), ("<", r"$<$"), (">", r"$>$")]
    for old, new in replacements:
        value = value.replace(old, new)
    return value


def write_tex(db: dict) -> None:
    instructions = sorted(db["instructions"], key=lambda x: (x["mnemonic"], x["syntax"]))
    body = []
    for inst in instructions:
        opcode = " ".join(f"{x:02X}" for x in inst["encoding"]["opcode"]) or "alias"
        body += [
            rf"\section{{{tex_escape(inst['mnemonic'])} -- {tex_escape(inst['syntax'])}}}",
            rf"\textbf{{Status:}} {tex_escape(inst['status'].upper())}\quad \textbf{{Feature:}} {tex_escape(inst['feature'])}\quad \textbf{{Privilege:}} {tex_escape(inst['privilege'])}\\",
            rf"\textbf{{Encoding:}} \texttt{{{opcode}}}, {tex_escape(inst['encoding']['format'])}\\",
            rf"\textbf{{Operand binding:}} {tex_escape(inst['encoding']['operand_binding'])}",
            "", tex_escape(inst["description"]) + ".", "", r"\subsection*{Operation}", r"\begin{lstlisting}",
            *inst["operation"], r"\end{lstlisting}", r"\subsection*{Flags}",
            ", ".join(f"{k}={v}" for k, v in inst["flags"].items()) + ".", r"\subsection*{Memory Ordering}",
            tex_escape(inst["memory_order"]), r"\subsection*{Exceptions}", r"\begin{itemize}",
            *[rf"\item {tex_escape(x)}" for x in inst["exceptions"]], r"\end{itemize}", r"\clearpage", "",
        ]
    preamble = r"""\documentclass[10pt]{article}
\usepackage[margin=0.8in,includefoot]{geometry}
\usepackage{fancyhdr}
\usepackage{hyperref}
\usepackage{listings}
\usepackage{xcolor}
\usepackage{titlesec}
\usepackage{tocloft}
\pagestyle{fancy}\fancyhf{}
\fancyhead[L]{\sffamily SeaBird Volume 2 -- Instruction Reference}
\fancyhead[R]{\sffamily Architecture 3.2 / SDK 1.0}
\fancyfoot[C]{\thepage}
\setlength{\headheight}{14pt}
\titleformat{\section}{\Large\bfseries\sffamily}{\thesection}{0.7em}{}
\titleformat{\subsection}{\normalsize\bfseries\sffamily}{}{0pt}{}
\cftsetpnumwidth{2.5em}
\setlength{\cftsecnumwidth}{3.5em}
\lstset{basicstyle=\ttfamily\small,breaklines=true,frame=single,backgroundcolor=\color{gray!8}}
\hypersetup{colorlinks=true,linkcolor=black,urlcolor=black}
\sloppy\emergencystretch=1em
\begin{document}
\begin{titlepage}\centering\vspace*{2cm}
{\Huge\bfseries SeaBird ISA\\[0.3cm]}{\LARGE Volume 2: Instruction Reference\\[0.4cm]}
{\Large Architecture 3.2 / SDK 1.0}\vfill
{\large Generated from the machine-readable architectural database\\August 14, 2026}\vfill
\end{titlepage}
\tableofcontents\clearpage
\section*{Status Legend}
NORMATIVE entries have complete machine-readable contracts. RESERVED entries are retired allocations that always raise INVALID\_OP and cannot be reused in v3. PROVISIONAL entries require further definition. ALIAS entries emit the canonical instruction.\clearpage
"""
    (DOCS_DIR / "volume-2-instruction-reference.tex").write_text(preamble + "\n".join(body) + "\n\\end{document}\n", encoding="utf-8")


def write_coverage(db: dict) -> None:
    counts = Counter(x["status"] for x in db["instructions"])
    category_counts: dict[str, Counter] = {}
    for inst in db["instructions"]:
        category_counts.setdefault(inst["category"], Counter())[inst["status"]] += 1
    lines = [
        "# SeaBird ISA Completion Ledger", "", "Generated by `tools/build_isa_spec.py`. Do not maintain counts by hand.", "",
        f"- Total entries: **{len(db['instructions'])}**", f"- Normative: **{counts['normative']}**",
        f"- Reserved: **{counts['reserved']}**", f"- Provisional: **{counts['provisional']}**", f"- Aliases: **{counts['alias']}**", "",
        f"- Performance markers: **{len(db['performance_markers']['markers'])}** (prefix modifiers; not instruction entries)", "",
        "| Category | Normative | Reserved | Provisional | Alias | Total |", "|---|---:|---:|---:|---:|---:|",
    ]
    for category, status in sorted(category_counts.items()):
        total = sum(status.values())
        lines.append(f"| {category} | {status['normative']} | {status['reserved']} | {status['provisional']} | {status['alias']} | {total} |")
    lines += ["", "## Performance Markers", "",
              "| ID | Modifier | Applicability |", "|---:|---|---|"]
    for marker in db["performance_markers"]["markers"]:
        lines.append(f"| {marker['id']} | `{marker['name']}.` | {marker['applicability']} |")
    lines += ["", "## Provisional Entries", ""]
    for inst in sorted((x for x in db["instructions"] if x["status"] == "provisional"), key=lambda x: (x["category"], x["mnemonic"])):
        lines.append(f"- `{inst['syntax']}` ({inst['category']}, `{inst['id']}`)")
    (DOCS_DIR / "ISA_COVERAGE.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    SPEC_DIR.mkdir(exist_ok=True)
    DOCS_DIR.mkdir(exist_ok=True)
    GEN_DIR.mkdir(exist_ok=True)
    rows = parse_allocations()
    db = build_database(rows)
    validate(db)
    (SPEC_DIR / "seabird-isa.json").write_text(json.dumps(db, indent=2) + "\n", encoding="utf-8")
    write_cpp(db)
    write_tex(db)
    write_coverage(db)
    counts = Counter(x["status"] for x in db["instructions"])
    print(f"entries={len(db['instructions'])} normative={counts['normative']} provisional={counts['provisional']} aliases={counts['alias']}")


if __name__ == "__main__":
    main()
