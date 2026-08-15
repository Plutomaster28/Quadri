# Pebble / SeaBird Project Map and WSL Migration Guide

This directory is a self-contained SeaBird ISA, reference-tooling, and LLVM 22
backend workspace. Copy the entire `pebble` directory into the Linux filesystem
inside WSL (for example `~/src/pebble`). Do not build the LLVM tree from
`/mnt/c/...`; native WSL storage is substantially better for a large C++ build.

## What must be preserved

Copy these directories. Together they contain the project inputs:

| Path | Purpose | Authority |
| --- | --- | --- |
| `spec/` | Machine-readable SeaBird ISA and architectural layouts | **Normative source of truth** |
| `clang/` | Native Clang 22 SeaBird target information and C ABI model | Hand-written frontend target overlay |
| `llvm/SeaBird/` | LLVM 22 SeaBird target backend | Hand-written backend plus generated opcode adapters |
| `llvm/patches/` | LLVM 22 integration patches for the triple and ELF support | Required for the LLVM source tree |
| `src/` | C++ x86-to-SeaBird translator and SeaBird reference model | Implementation/reference tooling |
| `runtime/` | Minimal CRT, string routines, and syscall assembly | Target runtime/bootstrap |
| `tests/` | ISA vectors and LLVM assembly/C tests | Verification inputs |
| `tools/` | Spec generators, validators, linker, and build/test scripts | Development tooling |
| `docs/` | Current ISA manuals, status ledgers, and Tritium profile sources | Current documentation sources |
| `generated/` | Generated C++ opcode metadata | Regenerable, but preserve for bootstrap/comparison |
| `README.md` | Concise project overview and current documentation entry points | Project introduction |
| `tritium_v1_datasheet.md` | Tritium embedded processor draft datasheet | Product/profile document |

The following are generated or temporary. They may be copied for reference, but
are not needed to reconstruct or build the project:

| Path | Contents |
| --- | --- |
| `build/` | Local compiler/test outputs |
| `output/` | Generated PDF manuals and LaTeX byproducts |
| `tmp/` | Rendered review images and temporary PDF builds |
| `pebble-xlate.exe`, `seabird-ref.exe` | Windows binaries; rebuild on Linux |
| `sample_out.*`, `sample_x86.bin` | Translator samples and generated output |
| `main (3).aux/.log/.out/.toc` | LaTeX build byproducts |
| `missfont.log` | LaTeX diagnostic |

## ISA documentation: what is what

There are several generations and kinds of ISA documentation. They should not
be treated as equally authoritative.

### 1. Normative machine-readable ISA

- `spec/seabird-isa.json` — SeaBird architecture version `3.2`
  instruction database. This is the canonical instruction registry: encodings,
  operand bindings, operations, flags, privilege, modes, exceptions, and status.
- `spec/isa.schema.json` — validation schema for the ISA database.
- `spec/architectural-layouts.json` — canonical feature bits, control-register
  layouts, query leaves, binary structures, ELF/ABI data, and related layouts.

When implementation code, generated manuals, and an old PDF disagree, start
with `spec/`.

### 2. Current generated manuals and prose source

- `docs/volume-1-basic-architecture.tex` — programming model, registers,
  calling convention, addressing, instruction formats, encoding, and flags.
- `docs/volume-2-instruction-reference.tex` — generated per-instruction
  reference derived from `spec/seabird-isa.json`.
- `docs/volume-3-system-programming.tex` — privilege, exceptions, paging,
  atomics, SMP, virtualization, and system architecture.
- `docs/volume-4-system-registers.tex` — system/control register reference.
- `docs/volume-5-binary-interfaces.tex` — feature/control layouts, binary
  structures, ELF relocations, TLS, DWARF, and ABI ratification material.
- `main (3).tex` — maintainable monolithic prose source from which Volumes 1,
  3, and 4 are extracted. Sections not selected by `tools/build_volumes.py`
  are legacy design material and are non-normative.
- `output/pdf/` — rendered versions of the current manuals. A PDF is current
  only when rebuilt from the matching generated `.tex` during the release run.

### 3. ISA status and ratification

- `docs/ARCHITECTURE_STATUS.md` — architecture-wide release gates and LLVM
  enablement status.
- `docs/DOCUMENTATION_INDEX.md` — authority order, current/historical boundary,
  and manual regeneration map.
- `docs/PAE32_EXTENSION.md` and `docs/REGISTER_WINDOWING_EXTENSION.md` —
  normative advanced-processor extension contracts.
- `docs/ISA_COVERAGE.md` — instruction completion ledger.
- `docs/V1_0_RATIFICATION_REPORT.md` — SDK 1.0 architecture and toolchain
  qualification results.
- `docs/RATIFICATION_REPORT.md` — v3.0 RC1 ratification results and reproduction
  notes.
- `tests/golden-vectors.json`, `tests/edge-vectors.json`, and
  `tests/ratification-vectors.json` — machine-checkable conformance material.

### 4. Tritium embedded profile (not the general ISA)

- `docs/tritium-embedded-profile/` — formal SeaBird Tritium embedded profile,
  including SB32 requirements, MPU, interrupt, lockstep, boot, and conformance.
- `tritium_v1_datasheet.md` — product-level Meisei Tritium draft datasheet.
- `output/pdf/seabird-tritium-embedded-profile.pdf` — generated reading copy.

Tritium is a constrained processor/profile built on SeaBird. Its documentation
does not replace the general SeaBird ISA volumes or the normative JSON database.

### 5. Historical/reference ISA material

- `SeaBird_Instruction_Set_Architecture (2) (2).pdf` — older imported ISA PDF.
- `seabird_isa_extracted.txt` — text extracted from that older PDF.
- `SeaBird_PAE_Reference.md` and `SeaBird_Register_Windowing_Reference.md` —
  superseded design proposals retained for provenance; their banners point to
  the ratified replacements.
- `main (3).pdf` — historical monolithic render; use the current volume PDFs.

Keep these for provenance and comparison. Do not use them as the current
machine-readable authority when they differ from `spec/seabird-isa.json`.

## LLVM backend layout

- `llvm/SeaBird/*.td` — target, registers, instructions, and calling convention.
- `llvm/SeaBird/AsmParser/` — assembly parser.
- `llvm/SeaBird/Disassembler/` — instruction decoder.
- `llvm/SeaBird/MCTargetDesc/` — MC encoding, printing, fixups, relocations,
  assembly backend, ELF writer, and target descriptions.
- `llvm/SeaBird/TargetInfo/` — target registration.
- `llvm/SeaBird/*ISel*`, `*Lowering*`, `*FrameLowering*`, and
  `*TargetMachine*` — SelectionDAG code generation and ABI lowering.
- `llvm/SeaBird/SeaBirdGenOpcodes.td` and
  `SeaBirdGenOpcodeMap.inc` — generated from the normative ISA database; do not
  assign opcode values here by hand.
- `llvm/patches/llvm-22-seabird-triple.patch` — LLVM target/triple integration.
- `llvm/patches/llvm-22-seabird-elf.patch` — LLVM ELF definitions/integration.
- `llvm/patches/clang-22-seabird-target.patch` — Clang target registration,
  target-info build integration, and driver CPU selection.
- `clang/SeaBird.h` and `clang/SeaBird.cpp` — project-owned native Clang target
  implementation copied into `clang/lib/Basic/Targets/SeaBird.*`.

The backend targets LLVM 22, specifically the selected `22.1.4` qualification
baseline. The first complete compiler profile is little-endian 64-bit
`SB-System64` using the `seabird64-unknown-none` triple. The MC layer also
supports the Tritium bring-up identity `seabird32-unknown-none` with CPU
`tritium-v1` and emits ELF32 objects. Native Clang uses the corresponding
LP64/ILP32 data models and emits both targets directly.

## WSL copy and first checks

From WSL, copy the directory into the Linux filesystem:

```bash
mkdir -p ~/src
cp -a /mnt/c/Users/theni/OneDrive/Documents/pebble ~/src/
cd ~/src/pebble
```

Confirm OneDrive has hydrated every file before deleting or disconnecting the
Windows copy:

```bash
find . -type f | wc -l
find . -type f -size 0
```

If a copied `.git` entry is an empty OneDrive reparse-point placeholder rather
than a usable repository, do not expect history to survive through it. Preserve
it under a backup name before initializing a new repository:

```bash
mv .git .git.placeholder-backup
git init
```

Only rename it after confirming you are in the intended project directory and
that it is not a usable Git repository.

## Linux prerequisites

Package names vary by distribution. On Ubuntu/Debian WSL, the basic toolset is:

```bash
sudo apt update
sudo apt install -y build-essential clang lld cmake ninja-build git python3 \
  python3-pip zip unzip xz-utils texlive-xetex
python3 -m pip install --user reportlab
```

LLVM itself is a separate, large source checkout. Use LLVM 22.1.4 to match the
patches and current backend.

```bash
mkdir -p /home/miyamii/llvm-src
git clone --depth 1 --branch llvmorg-22.1.4 \
  https://github.com/llvm/llvm-project.git \
  /home/miyamii/llvm-src/llvm-project-22.1.4
```

## Generate and validate project-owned files

The Python tools are portable and use project-relative paths:

```bash
cd ~/src/pebble
python3 tools/build_isa_spec.py
python3 tools/generate_llvm_tablegen.py --check
python3 tools/verify_ratification.py
```

Build the two standalone C++ tools without the Windows `.exe` suffix:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic src/main.cpp -o pebble-xlate
g++ -std=c++17 -O2 -Wall -Wextra -pedantic src/seabird_ref.cpp -o seabird-ref
```

`tools/run_conformance.py` currently names the outputs `*.exe`; Linux permits
that filename, so it can still run once its expected LLVM test outputs exist.

## Install the backend into LLVM on Linux

The existing PowerShell scripts contain Windows/MSYS paths. Their equivalent
native Linux installation is:

```bash
export PEBBLE=/home/miyamii/pebble
export LLVM_PROJECT=/home/miyamii/llvm-src/llvm-project-22.1.4
export LLVM_BUILD=/home/miyamii/llvm-build-seabird

mkdir -p "$LLVM_PROJECT/llvm/lib/Target/SeaBird"
cp -a "$PEBBLE/llvm/SeaBird/." \
  "$LLVM_PROJECT/llvm/lib/Target/SeaBird/"
cp "$PEBBLE/clang/SeaBird.h" \
  "$LLVM_PROJECT/clang/lib/Basic/Targets/SeaBird.h"
cp "$PEBBLE/clang/SeaBird.cpp" \
  "$LLVM_PROJECT/clang/lib/Basic/Targets/SeaBird.cpp"

git -C "$LLVM_PROJECT" apply \
  "$PEBBLE/llvm/patches/llvm-22-seabird-triple.patch"
git -C "$LLVM_PROJECT" apply \
  "$PEBBLE/llvm/patches/llvm-22-seabird-elf.patch"
git -C "$LLVM_PROJECT" apply \
  "$PEBBLE/llvm/patches/clang-22-seabird-target.patch"
```

Before applying a patch again, check whether it is already applied:

```bash
git -C "$LLVM_PROJECT" apply --reverse --check \
  "$PEBBLE/llvm/patches/llvm-22-seabird-triple.patch"
git -C "$LLVM_PROJECT" apply --reverse --check \
  "$PEBBLE/llvm/patches/llvm-22-seabird-elf.patch"
git -C "$LLVM_PROJECT" apply --reverse --check \
  "$PEBBLE/llvm/patches/clang-22-seabird-target.patch"
```

## Native LLVM build

```bash
cmake -S "$LLVM_PROJECT/llvm" -B "$LLVM_BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DLLVM_TARGETS_TO_BUILD= \
  -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=SeaBird \
  -DLLVM_ENABLE_PROJECTS=clang \
  -DLLVM_INCLUDE_TESTS=OFF \
  -DLLVM_INCLUDE_BENCHMARKS=OFF \
  -DLLVM_INCLUDE_EXAMPLES=OFF \
  -DLLVM_ENABLE_ZLIB=OFF \
  -DLLVM_ENABLE_ZSTD=OFF \
  -DLLVM_ENABLE_LIBXML2=OFF \
  -DLLVM_ENABLE_TERMINFO=OFF

cmake --build "$LLVM_BUILD" \
  --target clang llc llvm-mc llvm-tblgen llvm-readobj llvm-objdump \
    llvm-nm llvm-ar llvm-ranlib llvm-objcopy llvm-strip \
  --parallel 8
```

Basic registration check:

```bash
"$LLVM_BUILD/bin/llvm-mc" --version | grep -i seabird
"$LLVM_BUILD/bin/llc" --version | grep -i seabird
"$LLVM_BUILD/bin/clang" -target seabird64-unknown-none -dM -E -x c /dev/null \
  | grep SEABIRD
"$LLVM_BUILD/bin/clang" -target seabird32-unknown-none -mcpu=axium-m-v1 \
  -dM -E -x c /dev/null | grep -E 'SEABIRD_(PAE32|REGISTER_WINDOWS)'
```

After the integration suite passes, create the deterministic SDK archive with:

```bash
python3 tools/test_llvm_backend.py --llvm-build "$LLVM_BUILD"
python3 tools/run_conformance.py
python3 tools/record_toolchain_build.py \
  --llvm-project "$LLVM_PROJECT" --llvm-build "$LLVM_BUILD"
python3 tools/package_sdk.py --llvm-build "$LLVM_BUILD"
python3 tools/verify_sdk_package.py \
  dist/seabird-sdk-v1.0.0-marlin-linux-x86_64.zip
```

The portable `tools/install_llvm_backend.py`, `tools/build_llvm_backend.py`,
`tools/test_llvm_backend.py`, and `tools/compile_c_to_seabird.py` are the native
Linux/WSL workflow. The matching PowerShell scripts remain the Windows harness.

## Safe cleanup after a verified WSL build

Once the copy, generators, standalone tools, and LLVM build have been verified,
the following copied items may be removed from WSL to save space:

```text
build/
tmp/
*.exe
main (3).aux
main (3).log
main (3).out
main (3).toc
missfont.log
```

Keep `output/pdf/` if convenient access to rendered manuals matters. Keep all
legacy/reference documents until provenance is no longer needed.
