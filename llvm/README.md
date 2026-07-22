# SeaBird LLVM Backend

This directory contains the SeaBird-owned portion of an LLVM 22 backend. The first
profile is little-endian Dragonet (`SB-System64`) with 64-bit pointers and scalar GPR
code generation. It is deliberately staged: the MC layer and ordinary C ABI come
first, followed by floating-point/vector lowering and privileged extensions.

The companion `../clang/SeaBird.{h,cpp}` overlay defines the native Clang
frontend target. It provides SB64 LP64 and SB32 ILP32 data layouts, SeaBird CPU
and profile macros, inline-assembly registers/constraints, IEEE binary128
`long double`, and the pointer-form builtin `va_list`. The integration changes
for LLVM 22.1.4 are recorded in
`patches/clang-22-seabird-target.patch`.

`SeaBird/SeaBirdGenOpcodes.td` is generated from `spec/seabird-isa.json`:

```powershell
python tools/generate_llvm_tablegen.py
powershell -ExecutionPolicy Bypass -File tools/validate_llvm_tablegen.ps1
```

Do not assign opcode values in LLVM by hand. The generated records are an adapter
over the ratified ISA database, not a second instruction registry.

## Tritium bring-up

The backend recognizes the `seabird32-unknown-none` triple and `tritium-v1`
CPU. It shares mode-independent SeaBird encodings with the 64-bit target while
using ELF32 objects, 32-bit pointers and immediates, GPR32 SelectionDAG
patterns, and the SB32 integer calling convention. Call frames remain 16-byte
aligned, while overflow arguments use 4-byte slots.

`tests/llvm/tritium-mc.s` covers the assembler, disassembler, high-register
OREX, and ELF header path. `tests/llvm/tritium-codegen.ll` covers the first
end-to-end integer code-generation slice: arithmetic, ordered branches,
constants, loads/stores, calls, register arguments, stack arguments, and
software-pair 64-bit addition/subtraction, calls, and memory operations with
carry/borrow materialization. It also covers branchless variable shifts,
inline multiply using `MUL`/`MULH`, wide ordered comparisons, branchless
selection, boolean-controlled branches, and compiler-runtime calls for 64-bit
divide and remainder. The MC layer additionally round-trips the mandatory
two-register `DIV`, `MOD`, `UMUL`, `UDIV`, saturating arithmetic, NAND/NOR/XNOR,
rotate, min/max, and SGT family. SelectionDAG emits the applicable native i32
divide, remainder, saturation, rotate, and signed min/max instructions.

The generated slice contains all 310 instruction records from the
architecture's 310 normative entries. The MC layer exposes all 149 normative
SeaBird `BASE` mnemonics, all 10 scalar `FP` mnemonics, all 15 base `SIMD`
mnemonics, all 23 `SYSX` mnemonics, all 18 `FPX` mnemonics, all 10 normative
`CRYPTO` mnemonics, all 25 normative `DSP` mnemonics, all 7 `TXN` entries,
all 14 normative `ATOMICS` entries, all 39 normative `AVX` mnemonics, and all 125
mandatory Tritium mnemonics.
This includes immediate,
compare, control,
stack, bitfield/BMI, atomic/ordering, system-register, pair-memory, and SYSX
forms. `NEG`, `INC`, `DEC`, `NOT`, `ABS`, `CLZ`, `CTZ`, and `POPC` use their
parent SeaBird encodings; the applicable operations also have native SB32
SelectionDAG patterns.
SB64 SelectionDAG also selects scalar square-root/negate/absolute, fused
multiply-add/subtract, floating min/max, and vector
divide/absolute/signed-min/signed-max operations.
SB64 binary32 uses explicit `.s` assembly forms and supports arithmetic,
square root, negate/absolute, fused operations, min/max, scalar memory,
signed integer conversions, binary32/binary64 conversions, calls, returns,
spills, and stack arguments beyond V0-V7. Native C tests also cover the current
indirect ABI for small integer and binary32 structures passed and returned by
value.
IEEE ordered/unordered comparisons and floating selects are lowered for both
binary32 and binary64. The `FCMP` expansion distinguishes unordered, less,
equal, and greater outcomes through ZF/CF, including normal C NaN semantics.
Unsigned i64 conversions to/from binary32 and binary64 use correction
sequences around the signed conversion instructions and cover the 2^63
boundary.
SB64 binary128 uses canonical `.q` assembly and the same V-register ABI,
with 16-byte stack slots after V0-V7. Native `long double` arithmetic,
memory, calls, signed i64 conversions, IEEE comparisons, and selects compile,
round-trip through MC, link, and execute in the reference model.
SB64 atomic exchange IR also selects the native memory `XCHG` form.
SB64 and SB32 materialize local/global/external addresses with instruction
ABS64/ABS32 relocations. SB64 also lowers integer value comparisons and selects,
multiply/divide/remainder, bit counts, scalar/vector spills, and binary64
constant pools. Both object widths support ABI-preserving frame pointers and
aligned dynamic stack allocation. Large switches lower to comparison trees
until the target gains native jump-table support. The integrated assembler
accepts ordinary ELF/GNU directives,
the documented SeaBird integer-data aliases, diagnostics, and inline
`.cpu`/`.arch`/`.mode` profile controls.

Variadic calls keep named arguments in the ordinary ABI locations and place
unnamed arguments in pointer-width stack slots. The pointer-style `va_list`
lowering covers `va_start`, `va_arg`, `va_copy`, and `va_end`; tested SB64
integer/binary64 and Tritium integer functions compile successfully from both
LLVM IR and unchanged native C using Clang's `stdarg.h`.

MC coverage is not the same as complete compiler lowering. SelectionDAG still
needs binary16, soft-float, broader 64-bit software-pair operations, and
runtime support.
Native SB32 atomic IR selection covers loads/stores, compare-exchange (including
its success result), fetch add/subtract/and/or/xor, and fences. The ratified
`CMPXCHG Rexpected, Rdesired, [addr]` form binds the expected register and
address through ModR/M and the desired register through XOP0. The `tritium-v1`
processor bundle
rejects exposed parent-only instructions such as Octa/pair memory, pair stack,
FP, SIMD, SYSRET/process-ID, and SYSCALL. Division and remainder currently
require runtime definitions of `__divdi3`, `__udivdi3`, `__moddi3`, and
`__umoddi3`; those helpers must preserve the profile's divide-by-zero exception
contract.
