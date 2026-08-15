# SeaBird SDK 1.0 Ratification Report

Ratification date: 2026-08-14  
Linux toolchain qualification: 2026-08-15  
SDK: 1.0.0 Advanced Processor Edition  
Architecture: SeaBird 3.2

## Result

Architecture-owned conformance gates pass.

- 357 catalog entries
- 352 unique encoded allocations
- 325 normative instructions
- 27 reserved allocations
- 5 aliases
- 10 architecturally inert performance markers
- 325 exact-coverage golden vectors
- 40 independent arithmetic/FP/vector/crypto edge vectors
- 5 independent PAE32 translation vectors
- C++ reference window/marker tests pass warning-free
- Legacy example, C ABI, ELF, translator, and memory-model conformance passes
- LLVM 22.1.4 Linux x86_64 compiler/tool bundle passes the native integration
  suite with assertions enabled

## PAE32 gates

- 32-bit VA fields cover every bit exactly once.
- The 2/9/9 walk makes an L2 leaf cover exactly 2 MiB.
- PTE fields occupy one 64-bit entry without overlap.
- 36-bit high-physical mappings, large pages, write protection, XD, and
  physical-width faults have executable oracle cases.
- Existing ASID/TLB invalidation and shootdown operations are reused.

## Register-window gates

- Global, incoming, local, and outgoing classes partition R0--R31.
- Outgoing/incoming overlap and R8/R9-to-R24/R25 return propagation execute in
  the reference model.
- `WINNEW`, `WINPREV`, `WINRESERVE`, `WINPIN`, and `WINRELEASE` have unique
  encodings, exact golden vectors, feature-gated LLVM records, and MC fixtures.
- `reuse` and `leaf` are accepted only on calls and cannot change logical call
  semantics.
- Axium M LLVM lowering has separate caller/callee argument and result maps.
- ELF flags distinguish windowed/ordinary ABI and PAE32 requirements. The
  linker rejects ABI mixing while accumulating the compatible PAE environment
  requirement.

## Reproduction

```sh
python3 tools/run_conformance.py
python3 tools/generate_llvm_tablegen.py --check
```

The qualified build uses LLVM tag `llvmorg-22.1.4`, commit
`35990504507d79e0b9deb809c8ee5e1b34ceef20`. Its native integration suite
covers 23 assembly fixtures/demos, 17 IR fixtures, 23 C fixtures, Axium M frontend
macros and ELF flags, windowed-ABI lowering, LLVM object utilities, the static
and hosted runtime paths, and all ten example programs. Exact configuration and
tool hashes are shipped as `toolchain-build.json` in the SDK archive.

The Python static linker now emits flat or loadable ELF32 and ELF64 images.
Tritium and Axium final ELF32 executables are qualification cases, Axium
PAE/window flags are preserved, and mixed-class or incompatible-ABI inputs are
rejected. Arbitrary linker scripts, shared objects, and dynamic loading remain
outside this compact static linker's scope.
