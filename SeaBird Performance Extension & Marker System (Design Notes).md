# SeaBird Performance Extension & Marker System (Design Notes)

> **Non-normative design rationale.** The marker IDs, applicability, encoding,
> and required inert semantics are defined by `spec/seabird-isa.json` and
> Volume 1. This note explains intent and cannot override those sources.

## Overview

One of SeaBird's primary design philosophies is **explicitness**. Rather than forcing hardware to infer programmer intent, the ISA allows software (primarily the compiler) to communicate optimization opportunities directly.

This is inspired by the cooperation philosophy of MIPS, but modernized for contemporary compiler technology.

---

# Design Philosophy

SeaBird follows several core principles:

- Correctness is always preserved.
- Performance hints are optional.
- Hardware may ignore optimization hints while remaining ISA-compliant.
- Instructions describe *what* to do.
- Markers describe *how the compiler expects it to behave.*

This creates a clear separation between architectural behavior and implementation-specific optimization.

---

# Marker System

> Implementation status (SeaBird architecture v3.2 / SDK v1.0.0): the eight
> original markers in this note, plus the register-window `reuse` and `leaf`
> markers, are
> encoded as `FD <marker-id>` before the ordinary instruction encoding. Only
> one marker may modify an instruction. All defined markers are mandatory to
> decode and optional to optimize. See Volume 1 for the normative contract.

## Concept

A marker is an instruction modifier placed before an instruction.

Example:

```asm
assume.mov r0, r1
```

The instruction remains:

```asm
mov
```

The marker provides additional semantic information.

Markers are **not** new instructions.

Instead:

```
Marker + Instruction
```

becomes the execution unit.

---

# Purpose

Markers communicate programmer/compiler intent without changing program correctness.

Example categories include:

- assumption
- branch likelihood
- streaming memory accesses
- prefetch hints
- dependency hints
- register lifetime hints

---

# Rule

Markers must never change architectural correctness.

If a processor ignores every marker, the program must still execute correctly.

Markers only influence optimization opportunities.

---

# Example Marker Ideas

## Assume

```asm
assume.load r0, [r1]
```

Meaning:

"I expect this load to succeed."

If the assumption is incorrect:

- the instruction's ordinary precise exception or recovery behavior occurs
- no fault, privilege, ordering, or dependency rule is suppressed
- architectural correctness is preserved

---

## Likely

```asm
likely.branch loop
```

Compiler expects this branch to be taken.

Possible hardware use:

- branch prediction bias
- fetch prioritization

---

## Unlikely

```asm
unlikely.branch error
```

Compiler expects this path to be cold.

---

## Stream

```asm
stream.load r2, [r3]
```

Indicates data is likely read only once.

Possible hardware usage:

- cache policy adjustments
- prefetch behavior

---

## Prefetch

```asm
prefetch.load [r5]
```

Compiler predicts future memory access.

This modifier still performs the underlying load normally. It is distinct from
the standalone `PREFETCH [addr]` instruction, which has no destination value.

---

## Temporary

```asm
temporary.add r0, r1, r2
```

Result has a short expected lifetime.

Useful for processors implementing register renaming.

---

## Persistent

```asm
persistent.load r7, [r8]
```

Compiler expects the value to remain live for an extended period.

---

## Independent

```asm
independent.mul r4, r5, r6
independent.add r7, r8, r9
```

Compiler asserts no dependency exists between nearby operations.

Wide superscalar processors may exploit this.

---

# Relationship to Register Renaming

SeaBird exposes 32 architectural registers.

Future processors may internally implement many more physical registers.

Markers are **not** intended to expose physical registers directly.

Instead, markers provide information useful for:

- register renaming
- scheduling
- dependency tracking
- instruction issue

The processor remains free to implement any number of physical registers.

---

# Relationship to Register Windows

The optional `REGISTER_WINDOWS` extension defines architectural logical-window
behavior and an ABI. Physical window count, backing allocation, caching, and
register-renaming interaction remain implementation decisions.

Markers should complement windowing rather than replace it.

Compiler knowledge may assist implementations that utilize windows, but
markers never change the extension's logical transitions or correctness.

---

# Compiler Responsibilities

The compiler should emit markers only when confidence is sufficiently high.

Examples:

- proven type information
- profile-guided optimization
- speculative optimization
- alias analysis
- escape analysis
- loop optimization

---

# Hardware Responsibilities

Processors may:

- fully implement markers
- partially implement markers
- ignore markers entirely

All three remain ISA compliant.

---

# Benefits

- Cleaner ISA than introducing numerous specialized instructions.
- Better communication between compiler and processor.
- Allows simple and advanced processors to share one ISA.
- Provides future optimization opportunities without changing program semantics.
- Maintains backward compatibility.

---

# Long-Term Vision

SeaBird's defining characteristic is not simply being a hybrid CISC/RISC ISA.

Its defining philosophy is:

> Explicit cooperation between software and hardware.

Rather than forcing hardware to guess programmer intent, SeaBird allows the compiler to communicate expectations directly through markers.

This gives future implementations the freedom to exploit increasingly sophisticated optimization techniques while maintaining complete architectural compatibility.
