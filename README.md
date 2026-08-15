# SeaBird ISA and SDK

SeaBird architecture 3.2 and SDK 1.0.0 (`marlin`) define the complete base ISA,
PAE32, register windows, performance markers, LLVM/Clang target sources,
reference execution, and machine-readable conformance material.

Start here:

- `docs/PAE32_EXTENSION.md` — normative Tetra/SB32 physical-address extension.
- `docs/REGISTER_WINDOWING_EXTENSION.md` — normative window execution and ABI.
- `docs/volume-1-basic-architecture.tex` through
  `docs/volume-5-binary-interfaces.tex` — generated architecture manuals.
- `docs/TOOLCHAIN_GUIDE.md` — compiler, assembler, linker, and inspection usage.
- `docs/DOCUMENTATION_INDEX.md` — authority order and manual regeneration map.
- `examples/isa-showcase/` — buildable SB64, Axium PAE/window, and Tritium
  assembly tours spanning every major ISA capability family.
- `PROJECT_MAP_AND_WSL_MIGRATION.md` — source authority and Linux build layout.
- `docs/releases/v1.0.0-validation.md` — completed release gates and evidence.

The JSON files in `spec/` are authoritative for encodings and binary layouts.
Root documents explicitly marked superseded are retained only for provenance.
The Linux x86_64 SDK is qualified against LLVM 22.1.4 commit
`35990504507d79e0b9deb809c8ee5e1b34ceef20` with assertions enabled.

## Quick architecture checks

```sh
python3 tools/run_conformance.py
python3 tools/generate_llvm_tablegen.py --check
```

See the project map for exact LLVM 22.1.4 overlay, build, validation, packaging,
and WSL-native filesystem instructions.
