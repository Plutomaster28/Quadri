# SeaBird SDK Changelog

## v0.1.0 — Foundation Edition (2026-07-21)

Official standard-build tag: `tuna`  
License: Apache License 2.0

This is the first versioned developer release of the SeaBird ISA toolchain.
It establishes the software foundation on which SeaBird programs, emulation,
firmware, and future silicon can stand.

### Included

- Native LLVM 22 and Clang targets for SeaBird64 and SeaBird32/Tritium.
- Integrated assembler, disassembler, ELF object writer, and object inspector.
- All 310 normative SeaBird instructions in the MC layer.
- Freestanding SB64 C code generation with integer, binary32, binary64,
  binary128, vector, ABI, variadic, aggregate, and atomic coverage described in
  `docs/COMPILER_STATUS.md`.
- The Tritium-v1 embedded profile as a constrained SeaBird32 target.
- Static ELF64/raw linking and a minimal CRT/runtime foundation.
- Symbol, static-archive, binary-conversion, and strip utilities.
- Machine-readable ISA data, generated opcode metadata, executable golden
  vectors, an independent oracle, and the SeaBird reference model.

### Qualification baseline

- 310 normative golden instruction vectors pass.
- 34 independent encoding edge vectors pass.
- Compiled and linked C execution tests pass in the reference model.
- Raw binary and Intel HEX conversion, archive indexing, symbols, stripping,
  ELF32, and ELF64 object workflows are exercised.

### Known boundaries

- The custom linker is a static SeaBird64 foundation, not a complete system
  linker or dynamic loader.
- Tritium soft-float and qualified 64-bit divide/remainder runtime helpers are
  not complete.
- Binary16 compiler lowering, full PIC/GOT/PLT code generation, shared
  libraries, a standards-complete libc, C++ ABI qualification, exception
  handling, and production debugging workflows remain future work.
- ELF machine value `0x5342` remains experimental/private pending registration.
- v0.1 is a developer foundation release, not final silicon certification.

## v0.2 — Planned

The next release is expected to focus on the embedded runtime and platform
layer: Tritium soft-float and compiler-runtime completion, a packaged CRT and
minimal libc, improved linker behavior and scripts, binary16, hardware image
formats, debugging integration, and expanded emulator/silicon qualification.
