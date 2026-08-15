# SeaBird SDK v1.0.0 — marlin

This package is the SeaBird Advanced Processor Edition SDK, containing the
SeaBird 3.2 ISA, PAE32, register windows, performance markers, qualified
LLVM/Clang binaries, reference tooling, and conformance material.
`marlin` identifies the official standard build for this release. The SDK is
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

For Tritium, use `--target=seabird32-unknown-none -mcpu=tritium-v1`. For the
PAE32/register-window Axium profile, use
`--target=seabird32-unknown-none -mcpu=axium-m-v1`.

## Included commands

- `clang`, `llc`: compiler frontend and LLVM code generator
- `llvm-mc`: assembler
- `llvm-objdump`: disassembler
- `llvm-readobj`: ELF inspector
- `llvm-nm`: symbol inspector
- `llvm-ar`, `llvm-ranlib`: static archive tools
- `llvm-objcopy`: object-section binary and Intel HEX conversion
- `llvm-strip`: symbol/debug metadata removal
- `link_seabird.py`: static ELF32/ELF64 SeaBird linker and flat-image writer
- `seabird-ref`: executable ISA/reference-model and console-image runner
- `pebble-xlate`: x86-to-SeaBird translation prototype

The packaged Clang resource headers, minimal runtime sources, ISA database,
extension specifications, TeX/PDF manuals, examples, reference-model sources,
three conformance vector sets, status documents, release notes, and SHA-256
manifest are included.

This is the qualified architecture 3.2 / SDK 1.0 Linux x86_64 tool bundle. It
was built from LLVM 22.1.4 with assertions enabled and ships its exact source
commit, configuration, qualification results, and tool hashes in
`toolchain-build.json`. Read `VALIDATION.md` and `COMPILER_STATUS.md` before
deployment.

The loadable ELF writer emits a compact segment-oriented executable
without ordinary output sections. Use `link_seabird.py --format binary` for a
linked flat image; use `llvm-objcopy` on relocatable objects when extracting or
converting individual sections.
