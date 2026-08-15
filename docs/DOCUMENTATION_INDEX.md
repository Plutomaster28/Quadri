# SeaBird Documentation Index

Release: SDK 1.0.0 (`marlin`)  
Architecture: SeaBird 3.2

## Authority order

1. `spec/seabird-isa.json` is normative for instruction encodings and
   instruction-level behavior.
2. `spec/architectural-layouts.json` is normative for feature bits, control and
   system-register fields, PAE32, register-window images, ELF, XSAVE, and DWARF.
3. `docs/PAE32_EXTENSION.md` and `docs/REGISTER_WINDOWING_EXTENSION.md` are the
   normative prose contracts for the two advanced-processor extensions.
4. The five generated volume sources and their matching release PDFs are the
   consolidated programmer and implementer manuals.
5. Compiler and reference-model sources demonstrate implementations; they do
   not override the architecture.

When two same-revision documents disagree, the applicable JSON authority wins
and the disagreement is a release-blocking documentation defect.

## Current manuals

- Volume 1: basic architecture, programming model, ABI, encoding, and markers.
- Volume 2: generated reference for all 357 catalog entries and 325 normative
  instructions.
- Volume 3: privilege, exceptions, memory management, PAE32, synchronization,
  register-window system state, and virtualization.
- Volume 4: control and system-register reference.
- Volume 5: binary layouts, QUERY/control fields, window images, ELF, DWARF,
  and ratification requirements.

Volumes 1, 3, and 4 are extracted from selected sections of `main (3).tex`.
Volume 2 is generated from `spec/seabird-isa.json`. Volume 5 is generated from
`spec/architectural-layouts.json`. Generated volumes must not be edited by hand.

## Historical material

Files whose title or banner says `Superseded`, `Reference Draft`, or
`Historical` are provenance only. In particular, the root PAE proposal's
5/7/8 walk and conceptual MMU register names are not part of architecture 3.2.
The root performance-marker design note is current rationale but explicitly
non-normative; Volume 1 and the ISA JSON own its encoded contract.

## Toolchain and release documentation

- `docs/TOOLCHAIN_GUIDE.md`: SDK use, including SB64, Tritium, and Axium M.
- `PROJECT_MAP_AND_WSL_MIGRATION.md`: source-tree authority and LLVM 22.1.4
  overlay/build/package workflow.
- `docs/COMPILER_STATUS.md`: implemented lowering and explicit tool boundaries.
- `docs/releases/v1.0.0-validation.md`: release gates and their live status.
- `docs/V1_0_RATIFICATION_REPORT.md`: architecture-owned conformance evidence.

Architecture ratification and precompiled-compiler qualification are separate.
The former passes; the latter becomes complete only after the clean LLVM
22.1.4 build and integration suite recorded in the validation document.

## Regeneration

```sh
python3 tools/build_isa_spec.py
python3 tools/generate_golden_vectors.py
python3 tools/generate_llvm_tablegen.py
python3 tools/build_ratification_volume.py
python3 tools/build_volumes.py
python3 tools/verify_ratification.py
python3 tools/verify_documentation.py
```

Render every generated volume from the same tree, check that the TeX logs have
no errors, then run `python3 tools/record_documentation_build.py` to bind the
TeX and PDFs with SHA-256 hashes before packaging. The SDK packager
includes the normative extensions, volume sources, and matching PDFs.
