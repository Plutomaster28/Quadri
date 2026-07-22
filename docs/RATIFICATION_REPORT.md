# SeaBird ISA v3.0 RC1 Ratification Report

Date: 2026-06-27

## Result

**PASS - ratification candidate gates satisfied.**

- 342 catalog entries
- 337 unique encoded allocations
- 310 normative instructions
- 27 permanently reserved allocations
- 5 assembler aliases
- 0 provisional instructions
- 310 ID-bound golden vectors with exact normative-ID coverage
- 34 nonzero, edge, rounding, mask, restart, and known-answer vectors
- 4 TSO memory-order litmus contracts
- 2 byte-level OREX decoder vectors

## Independent Implementations

1. `src/seabird_ref.cpp` independently decodes and executes the scalar OREX core.
2. `tools/independent_oracle.py` evaluates the complete golden corpus without importing
   the specification generator or C++ reference model.
3. `src/main.cpp` consumes generated opcode metadata and is cross-checked for overlapping
   MOVI, ADD, OREX, little-endian immediate, and control-flow encodings.

## Machine-Checked Invariants

- Opcode and relocation numbers are unique.
- Feature bits and control-register bitfields do not overlap.
- IDT, context, XSTATE, and VMCB fields fit without overlap.
- Every normative instruction has an operand binding and exactly one baseline vector.
- Reserved allocations always raise `INVALID_OP` and cannot be reused in v3.
- Normative semantics contain no banned placeholder operations.
- Exception priority and TSO outcomes match their architectural contracts.
- All five PDF manuals compile without LaTeX errors or overfull boxes.

## Reproduction

```powershell
python tools/run_conformance.py
```

This command regenerates the instruction database and vectors, runs structural checks,
builds both C++ programs, executes the independent oracle, and cross-checks translator
bytes. Platform bindings and an official ELF registry assignment remain ecosystem work;
they do not alter the ratified ISA semantics or encodings.
