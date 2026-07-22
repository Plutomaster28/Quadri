# SeaBird SDK v0.1.0 — tuna

This package is the SeaBird Foundation Edition developer SDK.
`tuna` identifies the official standard build for this release. The SDK is
distributed under the Apache License 2.0; see `LICENSE`.

## Quick start

Add this package's `bin` directory to `PATH`, then compile a freestanding
SeaBird64 source file:

```sh
clang --target=seabird64-unknown-none -O2 -ffreestanding -fno-builtin \
  -c examples/c-smoke.c -o c-smoke.o
llvm-objdump -d c-smoke.o
llvm-readobj --file-headers --sections --symbols c-smoke.o
python3 bin/link_seabird.py --format elf --entry seabird_mix \
  -o c-smoke.elf c-smoke.o
python3 bin/link_seabird.py --format binary -o c-smoke.bin c-smoke.o
llvm-objcopy -O ihex --only-section=.text c-smoke.o c-smoke.hex
```

For Tritium, use `--target=seabird32-unknown-none -mcpu=tritium-v1`.

## Included commands

- `clang`, `llc`: compiler frontend and LLVM code generator
- `llvm-mc`: assembler
- `llvm-objdump`: disassembler
- `llvm-readobj`: ELF inspector
- `llvm-nm`: symbol inspector
- `llvm-ar`, `llvm-ranlib`: static archive tools
- `llvm-objcopy`: object-section binary and Intel HEX conversion
- `llvm-strip`: symbol/debug metadata removal
- `link_seabird.py`: current static SeaBird64 linker

The packaged Clang resource headers, minimal runtime sources, ISA database,
examples, status documents, release notes, and SHA-256 manifest are included.

This is a developer alpha. Read `RELEASE_NOTES.md` and `COMPILER_STATUS.md`
before selecting it for a production hardware or ABI freeze.

The current loadable ELF writer emits a compact segment-oriented executable
without ordinary output sections. Use `link_seabird.py --format binary` for a
linked flat image; use `llvm-objcopy` on relocatable objects when extracting or
converting individual sections.
