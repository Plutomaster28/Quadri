# SeaBird Toolchain Setup and Usage

This guide describes the SDK v1.0 workflow for compiling C, assembling
SeaBird source, linking programs, producing hardware images, inspecting files,
and running code in the reference model.

## 1. Install a qualified SDK

After the LLVM 22.1.4 validation record is complete, the Linux artifacts are
named `seabird-sdk-v1.0.0-marlin-linux-x86_64.zip` and the matching `.sha256`.
The intended release location is:

<https://github.com/Plutomaster28/Quadri/releases/tag/marlin>

Verify and unpack it:

```sh
sha256sum -c seabird-sdk-v1.0.0-marlin-linux-x86_64.zip.sha256
unzip seabird-sdk-v1.0.0-marlin-linux-x86_64.zip
export SEABIRD_SDK="$PWD/seabird-sdk-v1.0.0-marlin-linux-x86_64"
export PATH="$SEABIRD_SDK/bin:$PATH"
```

Confirm both targets are registered:

```sh
llvm-mc --version
clang --target=seabird64-unknown-none -dM -E -x c /dev/null |
  grep SEABIRD
```

The released Linux tools require a normal x86-64 glibc environment with
`libstdc++` and `libgcc_s`. Python 3 is required by the current static linker
and project build helpers.

## 2. Compile a freestanding C source

Compile directly to a SeaBird64 object:

```sh
clang --target=seabird64-unknown-none -O2 \
  -ffreestanding -fno-builtin -c program.c -o program.o
```

Keep intermediate forms when diagnosing compiler behavior:

```sh
clang --target=seabird64-unknown-none -O2 \
  -ffreestanding -fno-builtin -S -emit-llvm program.c -o program.ll
clang --target=seabird64-unknown-none -O2 \
  -ffreestanding -fno-builtin -S program.c -o program.s
```

For Tritium:

```sh
clang --target=seabird32-unknown-none -mcpu=tritium-v1 -O2 \
  -ffreestanding -fno-builtin -c firmware.c -o firmware.o
```

Tritium is the constrained embedded SeaBird32 profile. It is not a separate
unrelated ISA, and its compiler/runtime coverage remains narrower than SB64.

For Axium M v1 with PAE32 and the register-window ABI:

```sh
clang --target=seabird32-unknown-none -mcpu=axium-m-v1 -O2 \
  -ffreestanding -fno-builtin -c program.c -o program-axium.o
clang --target=seabird32-unknown-none -mcpu=axium-m-v1 \
  -dM -E -x c /dev/null | grep -E 'SEABIRD_(PAE32|REGISTER_WINDOWS)'
llvm-readobj --file-headers program-axium.o
```

The object must report `EF_SB_WINDOWED_ABI|EF_SB_PAE32_REQUIRED`. PAE32 keeps
32-bit pointers and changes only the required operating environment. The
window flag is an ABI identity: ordinary and windowed objects cannot be linked
directly.

## 3. Assemble handwritten SeaBird source

SeaBird64:

```sh
llvm-mc -triple=seabird64-unknown-none -filetype=obj \
  source.s -o source.o
```

Tritium:

```sh
llvm-mc -triple=seabird32-unknown-none -mcpu=tritium-v1 \
  -filetype=obj source.s -o source.o
```

Axium M v1:

```sh
llvm-mc -triple=seabird32-unknown-none -mcpu=axium-m-v1 \
  -filetype=obj source.s -o source.o
```

Buildable, commented architecture tours are provided in
`examples/isa-showcase/` for SeaBird64, Axium M PAE/register windows, and the
Tritium subset. They include privileged and arbitrary-address routines for
assembly/disassembly demonstration; those routines are not intended to be
executed linearly as an unprivileged application.

Canonical scalar floating spellings are unsuffixed for binary64, `.s` for
binary32, and `.q` for binary128.

## 4. Link an executable or flat image

The Python linker accepts homogeneous SeaBird32 or SeaBird64 ELF objects:

```sh
python3 "$SEABIRD_SDK/bin/link_seabird.py" \
  --format elf --entry main -o program.elf program.o
python3 "$SEABIRD_SDK/bin/link_seabird.py" \
  --format binary -o program.bin program.o
```

The ELF is the loadable/debuggable program container. The flat binary is the
contiguous linked image intended for the reference model, ROM tooling, or an
early hardware loader.

The SDK 1.0 linker is static and intentionally small. It emits flat images and
loadable ELF32/ELF64 executables, preserves Axium PAE/window flags, and rejects
mixed ELF classes or incompatible window ABIs. It is not yet a complete
replacement for a production linker with arbitrary scripts, shared objects,
GOT/PLT, or dynamic loading.

## 5. Inspect and disassemble files

```sh
llvm-readobj --file-headers --sections --symbols --relocations program.o
llvm-objdump -dr program.o
llvm-nm program.o
```

Create and inspect a static archive:

```sh
llvm-ar rcs libexample.a first.o second.o
llvm-ranlib libexample.a
llvm-ar t libexample.a
```

Strip debug metadata:

```sh
llvm-strip --strip-debug program.o -o program-stripped.o
```

## 6. Produce binary and HEX files

Extract an individual object section:

```sh
llvm-objcopy -O binary --only-section=.text program.o program-text.bin
llvm-objcopy -O ihex --only-section=.text program.o program-text.ihex
```

For a fully relocated flat program, use the linker's `--format binary` output.
`tools/build_example_programs.py` additionally creates a readable `.hex` dump
and an Intel HEX `.ihex` file from each linked image.

Do not run `llvm-objcopy -O binary` on the current sectionless loadable ELF:
that compact executable is segment-oriented, so the generic section extractor
has no output section to copy.

## 7. Build and run the compiler exercise suite

From the project checkout:

```sh
python3 tools/build_example_programs.py --llvm-build "$SEABIRD_SDK"
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  src/seabird_ref.cpp -o build/seabird-ref
python3 tools/test_example_programs.py --reference build/seabird-ref
```

Run one image manually:

```sh
build/seabird-ref --console \
  build/example-programs/add-two-numbers/add-two-numbers.bin \
  12 30
```

The arguments after the binary are scripted console tokens. If a token is not
provided, the reference model reads the next token from standard input.

## 8. Development-console ABI

The exercise programs use the small ABI in `runtime/seabird_console.h`.
`SYSCALL` receives its operation number in R0, the first argument in R1, and a
scalar result in R0:

| Number | Operation |
|---:|---|
| 1 | Read signed integer |
| 2 | Print signed integer from R1 |
| 3 | Print low byte of R1 |
| 4 | Print NUL-terminated string at address R1 |
| 5 | Read one character |

This interface is deliberately suitable for the reference model, early
emulators, FPGA monitors, and bring-up firmware. A future hosted OS ABI may use
a richer and separately versioned syscall contract.

## 9. Run the architecture/compiler conformance suite

```sh
python3 tools/run_conformance.py
```

The script regenerates and checks ISA vectors, builds the reference model, and
executes available compiler/linker artifacts. The broader LLVM integration
suite is `tools/test_llvm_backend.ps1`, used with a complete LLVM build tree.
