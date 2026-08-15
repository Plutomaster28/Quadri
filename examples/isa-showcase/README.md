# SeaBird ISA assembly showcase

These sources demonstrate the breadth of SeaBird architecture 3.2 while
remaining valid inputs to the release assembler:

- `seabird64-showcase.s` covers scalar integer/bit operations, data movement,
  SIB and specialized memory, synchronization, atomics, control flow,
  performance markers, IEEE floating point, SIMD/AVX, crypto/GF math, DSP,
  transactions, system/privileged/VM/security operations, and data sections.
- `axium-pae-window-showcase.s` covers the SB32 Axium M PAE32 environment,
  register-window operations and ABI markers, and MMU/TLB management.
- `tritium-showcase.s` covers the constrained Tritium SB32 subset.

The files are encoding tours. Routines containing privileged, VM,
transactional, or arbitrary-address memory operations are intentionally not a
single executable control path.

Assemble and inspect them with a qualified SDK:

```sh
llvm-mc -triple=seabird64-unknown-none -filetype=obj \
  seabird64-showcase.s -o seabird64-showcase.o
llvm-mc -triple=seabird32-unknown-none -mcpu=axium-m-v1 -filetype=obj \
  axium-pae-window-showcase.s -o axium-pae-window-showcase.o
llvm-mc -triple=seabird32-unknown-none -mcpu=tritium-v1 -filetype=obj \
  tritium-showcase.s -o tritium-showcase.o
llvm-objdump -d seabird64-showcase.o
llvm-readobj --file-headers axium-pae-window-showcase.o
```
