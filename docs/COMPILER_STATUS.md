# SeaBird Compiler and Assembler Status

This file separates instruction-set exposure from compiler lowering. An
instruction can be accepted by the assembler without LLVM yet knowing how to
select it from C/C++ or LLVM IR.

## Implemented

- 310/310 normative instructions have generated LLVM MC records.
- `llvm-mc` parses, encodes, and emits ELF32/ELF64 SeaBird objects.
- `llvm-objdump` disassembles base and extension encodings.
- Tritium is a `seabird32` CPU/profile of the shared target and rejects
  general-purpose-only instructions.
- Standard ELF/GNU sections, symbols, macros, conditionals, data emission,
  alignment, debug, CFI, and relocation directives use LLVM's generic parser.
- SeaBird adds `.word`, `.dword`, `.qword`, `.message`, `.cpu`, `.arch`, and
  `.mode`.
- SB32/SB64 lower scalar calls, stack arguments, fixed stack frames, branches,
  memory, globals, external symbols, constant pools, and static relocations.
- Clang 22 recognizes `seabird64-unknown-none` and
  `seabird32-unknown-none`, provides the LP64 and ILP32 C data models,
  defines SeaBird/profile preprocessor macros, validates SeaBird CPU names,
  and emits SeaBird LLVM IR, assembly, and ELF objects directly.
- The native Clang `stdarg.h` model uses the architectural pointer
  `va_list`; unchanged SB64 integer/binary64 and Tritium integer variadic C
  tests compile, link, and execute in the reference model.
- SB32/SB64 support aligned dynamic stack allocation with an ABI-preserving
  frame pointer.
- Variadic calls, formal variadic functions, `va_start`, `va_arg`, `va_copy`,
  and `va_end` use the mode-scaled pointer `va_list` ABI. SB64 supports tested
  integer, pointer-width, and binary64 variadic values; Tritium supports the
  tested integer path.
- Scalar and vector values can spill and reload under register pressure.
- SB64 lowers the tested integer, binary64, and v2i64 core operations.
- SB64 lowers binary32 arithmetic, square root, negate/absolute, fused
  multiply-add/subtract, min/max, loads/stores, signed integer conversions,
  binary32/binary64 conversions, calls, returns, and spills. Canonical assembly
  uses an explicit `.s` suffix; unsuffixed scalar FP remains binary64. Tested
  calls place excess binary32 arguments on the stack after V0-V7.
- Native SB64 C tests pass small integer and binary32 structures by value
  through Clang's current indirect aggregate ABI.
- SB64 lowers IEEE ordered/unordered comparisons and floating selects for
  binary32 and binary64. The lowering preserves NaN behavior by classifying
  all four `FCMP` outcomes through ZF/CF before materializing a C boolean.
- SB64 lowers unsigned i64 conversions to and from binary32/binary64 using
  correction sequences around the signed architectural conversion forms,
  including values at and above 2^63.
- SB64 models C `long double` as IEEE binary128 and lowers native `.q`
  arithmetic, square root, negate/absolute, fused operations, min/max,
  16-byte memory, calls/returns, spills, signed i64 conversions, IEEE
  comparisons, and floating selects. Binary128 arguments use V0-V7 followed
  by 16-byte-aligned stack slots.
- SeaBird ELF objects work with `llvm-readobj`, `llvm-nm`, `llvm-ar`/
  `llvm-ranlib`, `llvm-objcopy`, and `llvm-strip`. Tested conversion formats
  include raw binary and Intel HEX.
- Large switches lower to comparison trees until native jump-table support is
  added.
- SB32/Tritium lowers the tested native i32 and software-pair i64 core,
  including atomics and compiler-runtime divide/remainder calls.

## Remaining Compiler/ABI Work

- Add native binary16 lowering and qualify cross-format conversions involving
  binary128 where the architectural conversion contract permits them.
- Complete remaining Tritium soft-float and software-pair i64 operations, plus
  qualified `*di3` runtime helpers.
- Add full PIC/GOT/PLT/TLS code generation rather than only object/linker
  relocation support.
- Add native jump-table lowering, exception-handling/landing-pad support,
  register-class aggregate ABI classification and broader aggregate edge
  cases, C++ ABI qualification, and broader atomic lowering.
- Model optional extensions as independently selectable compiler/assembler
  features instead of only the full GP and Tritium profiles.
- Complete platform runtime work: syscall surface, startup variants, standard
  libraries, shared objects, lazy binding, and production ELF registration.

Historical source directives that claimed an assembler would autovectorize,
schedule, insert fences, or bypass safety checks are not implemented. Those are
compiler transformations or unsafe proposals, not object-format directives.
