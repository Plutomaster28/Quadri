# SeaBird v3 Architecture Status

This file tracks release gates that are broader than individual instruction entries.
Instruction-level counts are generated in `ISA_COVERAGE.md`.

## SDK Release Milestones

- [x] **v0.1.0 Foundation Edition (2026-07-21):** freeze and package the first
  developer baseline spanning Clang/LLVM, MC, ELF objects, the static linker,
  binary utilities, reference execution, and automated conformance.
- [ ] **v0.2:** complete the first embedded runtime/platform layer, including
  Tritium soft-float and compiler runtime, a minimal packaged C environment,
  stronger linking and image formats, binary16, debugging, and hardware-facing
  qualification.

## Completed

- [x] Operating modes, register widths, endianness, and subregister writes
- [x] GPR, vector, mask, floating-point, IP, FLAGS, and system state
- [x] Standard scalar/vector calling convention and C/C++ data models
- [x] ModR/M, SIB, OREX, VectorCtl, XOP, immediate, and displacement encoding
- [x] Unique opcode allocation validation
- [x] Precise exception priority, interrupt frames, privilege transitions, and reset state
- [x] Hierarchical paging, PTE format, ASIDs, TLB invalidation, and shootdown rules
- [x] TSO memory model, fences, atomics, LL/SC, and transaction fallback rules
- [x] SMP startup, IPIs, interrupt-controller contract, and timers
- [x] FP/SIMD state, IEEE behavior, masks, and context save/restore
- [x] Feature discovery and conformance profiles
- [x] Debug, PMU, power-state, machine-check, virtualization, and security contracts
- [x] A-Z instruction ledger: 310 normative, 27 reserved, 5 aliases, 0 provisional
- [x] Generated C++17 opcode metadata consumed by the translator
- [x] Split Volume 1-4 LaTeX and PDF manuals

## Separate Platform ABI Work

These are intentionally platform specifications rather than ISA semantics. They do not
change instruction behavior or encodings, but a production software ecosystem must select
one profile before binaries can be exchanged between operating systems.

- [ ] Register an official ELF `e_machine` value; use a private experimental value meanwhile
- [x] Freeze experimental numeric ELF relocation IDs for ABS, PC-relative, GOT, PLT, and TLS relocations
- [x] Freeze DWARF register numbers and unwind augmentation strings
- [ ] Publish firmware memory-map and hardware-description bindings
- [ ] Publish concrete interrupt-controller and timer MMIO bindings for the first board
- [x] Publish the first virtual-machine control-block binary layout
- [ ] Publish the cross-version virtual-machine migration format

## LLVM Toolchain Enablement

- [x] Define the SB-System64 LLVM register file and integer calling convention in TableGen
- [x] Generate scalar-core LLVM opcode records from the normative ISA database
- [x] Validate register, instruction, calling-convention, subtarget, and asm-writer tables with LLVM 22
- [x] Register the `seabird64` triple and experimental target components in LLVM 22
- [x] Register the `seabird32` triple and Tritium-v1 MC/ELF32 components in LLVM 22
- [x] Register native Clang SB32/SB64 targets with ILP32/LP64 data models, CPU/profile macros, inline-assembly registers, and pointer `va_list`
- [x] Compile unchanged `stdarg.h` C directly to SeaBird ELF32/ELF64 and execute the SB64 integer/binary64 variadic paths
- [x] Implement MC parsing/printing for the initial scalar instruction slice
- [x] Implement MC byte encoding and OREX-aware disassembly for the initial scalar slice
- [x] Emit ELF64 SeaBird objects with e_machine 0x5342 and RELA PCREL32 fixups
- [x] Compile optimized freestanding C leaf functions through SelectionDAG to SeaBird assembly and bytes
- [x] Execute C-generated raw SeaBird functions in the independent reference model
- [x] Implement direct calls, stack frames, 64-bit memory operations, and EQ/NE conditional SelectionDAG lowering
- [x] Compile, statically link, and execute freestanding multi-object C smoke programs in the reference model
- [x] Lower all signed/unsigned ordered integer branches, indirect calls, and stack arguments
- [x] Parse, encode, disassemble, and execute full base+index*scale+disp SIB addressing
- [x] Lower and execute binary64 arithmetic and 128-bit v2i64 arithmetic/logic
- [x] Link text/rodata/data/BSS with ABS16/32/64 and PCREL32 relocations
- [x] Emit loadable ELF64 executables and run CRT-to-main hosted syscall smoke programs
- [x] Add RC2 scalar FLD/FST semantics and binary64 LLVM lowering
- [x] Emit page-separated RX/R/RW ELF segments with file-free BSS tails
- [x] Emit PT_TLS and resolve R_SB_TLS_LE static TLS relocations
- [x] Apply RELATIVE, GLOB_DAT, and JUMP_SLOT dynamic-compatible relocations
- [x] Compile and execute bootstrap memset/memcpy/strlen libc routines
- [x] Lower 128-bit vector memory and signed i64/binary64 conversions
- [x] Add initial Tritium SB32 SelectionDAG lowering, ILP32 scalar calls, and 16-byte-aligned call frames
- [x] Lower SB32 software-pair 64-bit add/subtract, calls, and memory operations using Tritium XOR/SLT carry and borrow materialization
- [x] Lower SB32 software-pair variable shifts and inline multiply using deterministic scalar sequences
- [x] Lower SB32 software-pair ordered comparisons, branchless selections, and boolean-controlled branches
- [x] Expose the Tritium binary R-type divide, saturation, logic, rotate, min/max, and SGT family and select its applicable native i32 operations
- [x] Expose shared SeaBird/Tritium unary and bit-count encodings and select native SB32 NEG/INC/DEC/NOT/ABS/CLZ/CTZ/POPC operations
- [x] Expose all 149 normative SeaBird BASE mnemonics in the generated MC layer
- [x] Expose all 10 scalar FP and all 15 base SIMD mnemonics in the generated MC layer
- [x] Expose all 23 architectural SYSX mnemonics, including PIO, XSTATE, MMU, SMP, virtualization, IBT, shadow-stack, PMU, RNG, context, and mode-control forms
- [x] Expose all 18 FPX mnemonics and select native SB64 fused multiply-add/subtract and floating minimum/maximum operations
- [x] Expose all 10 normative CRYPTO mnemonics with high-register vector encodings
- [x] Expose all 25 normative DSP mnemonics with GPR, immediate, multi-XOP, and mixed GPR/vector encodings
- [x] Expose all 7 normative TXN entries, including both XABORT operand variants
- [x] Expose all 14 normative ATOMICS entries and select native SB64 atomic exchange
- [x] Expose all 39 normative AVX mnemonics with canonical `FF 01`, vector, immediate, gather, and scatter encodings
- [x] Expose all 310 normative SeaBird instructions in the generated MC layer
- [x] Implement documented ELF/GNU data, section, symbol, diagnostic, profile, CPU, and object-mode assembler directives
- [x] Add SB32/SB64 global, external-symbol, and constant-pool address materialization with ABS32/ABS64 instruction relocations
- [x] Add scalar and vector register spill/reload support across calls and under register pressure
- [x] Add an ABI-preserving frame pointer and aligned SB32/SB64 dynamic `alloca` lowering
- [x] Define and lower the mode-scaled pointer `va_list` ABI, including stack-only unnamed arguments, `va_start`, `va_arg`, `va_copy`, and `va_end`
- [x] Lower SB64 integer comparisons, boolean materialization, and branchless selects
- [x] Lower SB64 multiply, signed/unsigned divide, signed/unsigned remainder, absolute value, CLZ, CTZ, and population count
- [x] Select native SB64 scalar square-root/negate/absolute and vector divide/absolute/signed-min/signed-max operations
- [x] Lower and execute native SB64 binary32 arithmetic, memory, calls, signed conversions, binary32/binary64 conversions, and small indirect aggregates
- [x] Lower and execute IEEE ordered/unordered binary32 and binary64 comparisons and floating selects, including NaN behavior
- [x] Lower and execute unsigned i64 conversions to/from binary32 and binary64 across the 2^63 boundary
- [x] Lower and execute native IEEE binary128 arithmetic, memory, calls, signed conversions, comparisons, and selects with canonical `.q` assembly
- [x] Qualify SeaBird ELF symbol, archive, raw/Intel-HEX conversion, and strip operations with LLVM's generic utilities
- [x] Expose all 125 Tritium mandatory mnemonics, including immediate, stack, bitfield, atomic/ordering, system, and SYSX families
- [x] Encode and disassemble active-width immediates, stack/control forms, scalar XOP operands, and SYSX `FF 04` streams
- [x] Ratify `CMPXCHG Rexpected, Rdesired, [addr]` with ModR/M expected/address and XOP0 desired bindings and add its Tritium MC record
- [x] Select native SB32 atomic loads/stores, compare-exchange (including its success result), fetch add/subtract/and/or/xor, and fence operations
- [x] Lower SB32 64-bit divide/remainder to the standard compiler-runtime ABI
- [ ] Implement and qualify Tritium `*di3` divide/remainder runtime helpers, including divide-by-zero behavior
- [x] Make `tritium-v1` reject excluded GP-only and optional-extension records, including AVX, in the shared MC layer
- [ ] Complete Tritium soft-float/remaining software-pair coverage and finer optional-extension feature gating
- [ ] Add binary16 lowering, shared-object symbol tables, lazy binding, complete syscalls, and a standards-complete libc

## Ratification Test Expansion

- [x] Machine-check opcode uniqueness, reserved encodings, bitfield overlap, and structure bounds
- [x] Reference decode/execute tests for OREX, MOVI, ADD, SUB, and integer FLAGS
- [x] Exception-priority and TSO litmus contracts
- [x] Add ID-bound golden vectors for every normative instruction
- [x] Add IEEE-754 operation coverage and all four rounding modes
- [x] Add vector mask, restart, gather, and scatter fault vectors
- [x] Add known-answer baselines for every normative cryptographic instruction
- [x] Run the suite against a Python implementation independent of the specification generator and C++ model

## Release Rule

No instruction may move to NORMATIVE unless the database includes its encoding, operand
binding, operation, flags, memory ordering, privilege, modes, and exceptions. Reserved
allocations always raise `INVALID_OP` and cannot be reused within major version 3.
