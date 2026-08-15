# SeaBird ISA Family Audit

Audit date: 2026-08-14  
Architecture: SeaBird 3.2 (SDK 1.0)

Architecture 3.2 closes the advanced-processor profile with the optional
PAE32 translation regime and register-window ABI. PAE32 reuses the existing
MMU/TLB instruction surface; register windowing adds five normative SYSX
operations and two call-only performance markers.

This audit compares the normative instruction database, LLVM MC exposure,
compiler lowering, reference semantics, and reserved allocations. Instruction
counts below exclude aliases and performance markers.

| Requested family | Existing coverage | Audit result |
|---|---|---|
| SIMD / Vector | 15 base SIMD and 44 normative AVX instructions | Strong arithmetic, logic, memory, permutation, reduction, gather/scatter, and masking coverage. v3.1 adds equality, inequality, unsigned comparisons, and `VNOT`. Per-lane format conversion and masked memory remain reserved pending complete contracts. |
| Integer / ALU / Bit Manipulation | 27 arithmetic, 18 logic/bit, 16 BMI-style, and 10 comparison instructions | Strong scalar core. v3.1 adds `ADC`, `SBB`, and `UMULH`, closing the primary multi-precision gaps. A general flag-driven value-select instruction remains desirable. |
| Data Movement / Conversion | 23 data-movement instructions plus scalar/vector FP conversion and packing operations | `MOVZX`, `MOVSX`, and `MOVSWP` already cover extension and byte-order conversion. v3.1 adds direct unsigned integer/FP conversion. Vector format conversion remains reserved because its lane/rounding encoding is not yet fixed. |
| Control Flow | 22 branch/control and 10 stack/frame instructions | Complete direct/indirect call, return, signed/unsigned flag branches, register-zero branches, trap, and yield surface. Conditional value selection is the main remaining compiler-facing gap. |
| Floating Point | 30 normative scalar FP/FPX instructions | Binary32, binary64, and binary128 arithmetic, fused operations, compare, classify, rounding, loads/stores, signed conversion, and now unsigned conversion are covered. Binary16 lowering remains incomplete. |
| Crypto / Hash / GF math | 10 normative and 14 reserved instructions | AES round transforms, inverse mix, carryless multiply, GHASH/GF(2^128), SHA-1 schedule, and SHA-256 sigma helpers exist. Coverage is incomplete for production hash acceleration: AES key generation and SHA-256 rounds remain reserved because their old syntax lacks required operands; SHA-512/SHA-3/BLAKE/SM3 are absent. |
| System / Privileged / VM | 15 base system, 23 SYSX, and 7 transactional instructions | Strong privilege, interrupt, timing, context, MMU/TLB, SMP/IPI, virtualization, shadow-stack, PMU, RNG, and mode-control coverage. No foundational omission was identified in this pass. |
| Memory / Cache / Synchronization | Scalar/vector loads and stores, 11 memory extensions, 14 atomics, four fences, cache/TLB maintenance | Broad coverage under the TSO model. The audit corrected several false “No memory access” contracts. Explicit weak-memory acquire/release forms are unnecessary for current TSO but would be required by any future relaxed-memory profile. Atomic min/max are optional future candidates. |
| String / Specialized Memory | `CPYB`, `CPYW`, and `MEMFILL`, plus auto-increment/decrement loads/stores and pair accesses | Copy/move and fill are present with restart contracts. Their memory ordering, exceptions, and LLVM side effects are now corrected. Bounded compare, byte search, and length operations remain candidates after their progress/fault result ABI is specified. |

## Changes made by this audit

- Allocated base opcodes `D8`--`DC` to `ADC`, `SBB`, `UMULH`, `FCVTU`, and
  `FCVTUS`.
- Allocated AVX sub-opcodes `2B`--`2F` to four missing predicate operations and
  `VNOT`.
- Added generated LLVM MC records for all ten instructions.
- Added native SB64 selection for unsigned high multiply and unsigned
  integer/FP conversions.
- Corrected memory contracts and LLVM `mayLoad`/`mayStore` modeling for the
  specialized string instructions.
- Corrected `FCVTS` and `FCVTUS` to specify truncation toward zero, matching
  LLVM integer-conversion semantics and the reference implementation.
- Added golden vectors, independent edge cases, and reference execution for
  the new scalar operations.

## Recommended next architecture work

1. Define one general conditional-select encoding rather than a large family of
   condition-specific moves.
2. Ratify `VCONVERT` only after fixing source/destination lane formats,
   saturation, rounding, masking, and exception behavior in the encoding.
3. Replace the reserved crypto helper syntax with explicit operands and
   immediates before promoting AES key-schedule or SHA compression operations.
4. Specify bounded `MEMCMP`, `MEMCHR`, and `STRNLEN` progress/fault results
   before allocating string opcodes.
5. Add LLVM carry-chain selection for `ADC`/`SBB` and qualify the new native
   unsigned conversion paths in a full LLVM 22 build.
