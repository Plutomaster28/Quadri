# SeaBird SDK Changelog

## 1.0.0 — Advanced Processor Edition (2026-08-14)

- Ratified the PAE32 translation regime: 32-bit VA, 36-bit PA, 64-bit PTEs,
  4 KiB/2 MiB pages, ASIDs, precise reason-coded faults, and Axium M v1's
  48 GiB platform limit.
- Ratified the opt-in register-window ABI, transparent spill/restore state,
  exception/context contracts, five `WIN*` operations, and `reuse`/`leaf` call
  markers.
- Added the `axium-m-v1` Clang/LLVM CPU, PAE/window feature macros, windowed
  argument/result lowering, instruction gating, and ELF ABI flags.
- Added executable PAE walk vectors, reference window semantics, complete MC
  fixtures, generated manuals, and dedicated normative extension references.
- Made ELF compatibility precise: ordinary/windowed ABI objects cannot be
  mixed, while compatible PAE environment requirements are accumulated. LLVM
  now models stack and transparent window memory effects without treating R27
  as a nonexistent link register.
- Froze PAE32 memory-type numbers and the complete register-window privileged
  state, reset/enable rules, zero-initialization, spill-record ordering,
  portable XSAVE image, and privileged ABI-thunk contract.
- Reconciled documentation authority, marked proposal drafts as superseded,
  updated the marlin/Axium/Linux workflows, and regenerated all five release
  manuals.
- Rebuilt and qualified the Linux x86_64 compiler/tool package from LLVM 22.1.4
  commit `35990504507d79e0b9deb809c8ee5e1b34ceef20` with assertions enabled;
  native assembly, IR, C, utilities, linker/runtime, and example suites pass.
- Completed static ELF32 linking for Tritium and Axium M, including loadable
  executable headers, native-width relocations, PAE/window flag propagation,
  mixed-class rejection, and packaged ISA showcase sources.

### Included v3.1 performance and family-audit work

- Added eight architecturally inert performance markers using the `FD id`
  prefix and modifier syntax (`assume.`, `likely.`, `unlikely.`, `stream.`,
  `prefetch.`, `temporary.`, `persistent.`, and `independent.`).
- Added assembler applicability diagnostics, encoding/disassembly round trips,
  reference-model decoding, marker capability discovery, and Tritium's
  decode-and-ignore contract.
- LLVM code generation now transfers branch weights of at least 80/20 into
  `likely`/`unlikely` markers while leaving weakly biased branches unmarked.
- Corrected unconditional-branch barrier metadata and marker-aware branch and
  frame-immediate disassembly offsets.
- Audited all major instruction families and added ten foundational operations:
  `ADC`, `SBB`, `UMULH`, `FCVTU`, `FCVTUS`, `VCOMPARE_EQ`, `VCOMPARE_NE`,
  `VCOMPARE_ULT`, `VCOMPARE_UGT`, and `VNOT`.
- Corrected memory-order/exception metadata for string, stack, call/return,
  gather/scatter, cache-maintenance, and context/virtualization memory access;
  string operations now carry accurate LLVM memory side-effect descriptors.
- Corrected scalar IEEE-to-integer conversion semantics to require truncation
  toward zero, aligning `FCVTS`/`FCVTUS`, LLVM IR, and the reference model.

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

## Post-1.0 roadmap

The next feature release is expected to focus on the embedded runtime and platform
layer: Tritium soft-float and compiler-runtime completion, a packaged CRT and
minimal libc, improved linker behavior and scripts, binary16, hardware image
formats, debugging integration, and expanded emulator/silicon qualification.
