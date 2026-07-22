# Meisei Tritium — Embedded Processor
## Preliminary Datasheet (v1 Draft)

*High-end embedded core, SeaBird ISA (Tritium safety-critical subset)*

---

## 1. Overview

Tritium is the first member of Meisei's embedded processor family (alongside the planned Deuterium and Protium models), implementing the SeaBird ISA in its safety-critical, deterministic subset. Tritium targets industrial, automotive, and aerospace applications where worst-case execution time (WCET) provability and fault tolerance are required.

---

## 2. Core Architecture

| Feature | Specification |
|---|---|
| ISA | SeaBird — Tetra mode (32-bit) |
| Execution model | In-order, single-issue |
| Pipeline | 5-stage (Fetch / Decode / Execute / Memory / Writeback) |
| Speculation | None — no branch prediction, no OoO, no register renaming |
| Multiply/Divide | Hardware, fixed-latency |
| Interrupt model | Fixed-priority vectored (v1); nested vectored interrupts under R&D for future revision |
| Worst-case interrupt latency | 12 cycles (target) |

---

## 3. Memory Subsystem

| Feature | Specification |
|---|---|
| Instruction/Data memory | TCM (Tightly-Coupled Memory) only — no cache |
| Scratchpad | Large on-package scratchpad, implemented as separate add-on die from the logic die |
| Memory protection | MPU (region-based), standard on all units |
| ECC | SEC-DED, standard on memory interfaces |

---

## 4. Redundancy & Fault Tolerance

**Configuration:** Dual-core lockstep with hardware comparator

### Fault Detection & Response Flow

1. Both cores execute identical instruction stream in parallel
2. Architectural state compared before commit
3. **Match** → commit result, continue execution
4. **Mismatch** →
   - Commit stage halted on both cores (pipeline continues clocking; only commit freezes)
   - In-flight instructions discarded
   - Hardware fault supervisor latches: fault reason, PC, status, timestamp/cycle count, pipeline stage of divergence
   - `FAULT_OUT` asserted (level-held until cleared by software)
   - Control handed to software supervisor

### Supervisor Model

| Layer | Role |
|---|---|
| Hardware supervisor | Minimal fault-control circuit — freeze, latch, signal. Designed for simplicity/verifiability. |
| Software supervisor | Recovery policy and diagnostics — reset, safe mode entry, switchover, logging |

### Recovery

- Discard-and-refetch from latched PC (chosen over resume-in-place for verification simplicity and guaranteed clean state)

### Escalation Policy

- 2 lockstep faults within a defined time window → immediate safe mode entry, no retry loop

---

## 5. Clocking

| Feature | Specification |
|---|---|
| Clock scheme | Fixed, conservative frequency for v1 |
| Future direction | Discrete selectable frequency steps under evaluation for later revisions (deferred to preserve WCET provability in v1) |

---

## 6. Physical / Process

| Feature | Specification |
|---|---|
| Process node | SKY130 (130nm), SkyWater |
| Flow | OpenLane |
| Package | High pin-count (co-processor / peripheral expansion headroom) |

---

## 7. Toolchain

| Feature | Specification |
|---|---|
| Compiler | LLVM backend (SeaBird target) — C/C++ support functional |
| RTOS | FreeRTOS-class support planned (MPU-aware) |

---

## 8. Open Items for Future Revisions

- [ ] Finalize worst-case interrupt latency figure through post-synthesis timing analysis
- [ ] Evaluate nested vectored interrupts as opt-in mode
- [ ] Formal verification of hardware fault supervisor
- [ ] Discrete clock-step evaluation (post-v1)
- [ ] Target certification tier (ISO 26262 ASIL / DO-178C DAL) — TBD, affects future lockstep + diagnostic coverage requirements

---

*This document reflects design decisions as of the current planning stage and is subject to change prior to tapeout.*
