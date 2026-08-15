# SeaBird Register Windowing Extension — Superseded Reference Draft

> **Non-normative and superseded.** This proposal is retained for design
> provenance. Final encodings, state layouts, exception behavior, and ABI rules
> are in `docs/REGISTER_WINDOWING_EXTENSION.md`,
> `spec/architectural-layouts.json`, and the generated ISA volumes.

## 1. Purpose

The SeaBird Register Windowing extension provides a larger effective
register namespace across procedure calls while preserving the base
ISA's 32 visible architectural register names.

The design follows a hybrid principle:

> **Hardware owns correctness, allocation, spill/restore, and safety.
> Software supplies intent and optimization hints.**

Ordinary programs should not need to manually manage physical register
windows. Compilers, runtimes, and operating systems may provide
additional information when doing so improves execution.

This extension is architecturally separate from out-of-order register
renaming.

------------------------------------------------------------------------

## 2. Architectural Model

SeaBird continues to expose:

``` text
R0-R31
```

Register-window-capable implementations maintain multiple backing
windows:

``` text
Window 0: R0-R31
Window 1: R0-R31
Window 2: R0-R31
...
```

The number of physically resident windows is
**implementation-specific**.

A small SeaBird implementation might maintain only a few windows, while
an Axium M implementation may maintain substantially more. Software must
not depend on a particular physical window count.

------------------------------------------------------------------------

## 3. Suggested Register Classes

The initial windowed ABI divides the 32 architectural registers into
four groups:

``` text
R0-R7      Global / shared
R8-R15     Incoming
R16-R23    Local
R24-R31    Outgoing
```

### Global Registers

`R0-R7` remain associated with the execution context across ordinary
window changes.

### Incoming Registers

`R8-R15` contain arguments or values passed into the current procedure.

### Local Registers

`R16-R23` belong to the current procedure's window.

### Outgoing Registers

`R24-R31` are used to pass values to a called procedure.

------------------------------------------------------------------------

## 4. Window Overlap

Adjacent windows overlap logically.

The outgoing registers of a caller become the incoming registers of its
callee:

``` text
Caller                       Callee

R24 -----------------------> R8
R25 -----------------------> R9
R26 -----------------------> R10
R27 -----------------------> R11
R28 -----------------------> R12
R29 -----------------------> R13
R30 -----------------------> R14
R31 -----------------------> R15
```

This allows up to eight register arguments to cross a normal windowed
call without explicit register-to-register copies.

Example:

``` asm
mov     r24, 10
mov     r25, 20
call    add_values
```

Inside `add_values`:

``` asm
; caller R24 is visible here as R8
; caller R25 is visible here as R9

add     r16, r8, r9
ret
```

------------------------------------------------------------------------

## 5. Automatic Window Allocation

A normal windowed procedure call requests a new architectural window.

Conceptually:

``` asm
call foo
```

may perform:

``` text
1. Preserve required call state.
2. Advance to a new window.
3. Map caller outgoing registers to callee incoming registers.
4. Begin execution at foo.
```

A return reverses the relationship:

``` asm
ret
```

Conceptually:

``` text
1. Leave the current window.
2. Restore the caller's window relationship.
3. Resume execution at the return address.
```

The exact physical window selected is not visible to application
software.

------------------------------------------------------------------------

## 6. Hardware Responsibilities

The register-window controller is responsible for:

-   Tracking active window relationships.
-   Allocating resident windows.
-   Mapping caller outgoing registers to callee incoming registers.
-   Restoring previous windows on return.
-   Detecting resident-window exhaustion.
-   Automatically spilling windows when required.
-   Automatically restoring spilled windows.
-   Maintaining correctness across exceptions and interrupts.
-   Cooperating with context switching.
-   Preventing software optimization hints from corrupting architectural
    state.

The implementation may optimize any of these mechanisms as long as
architectural behavior remains correct.

------------------------------------------------------------------------

## 7. Software Responsibilities

Normal software does **not** manage physical window numbers.

Compilers and runtimes may provide hints concerning:

-   Leaf functions.
-   Window reuse.
-   Expected nested calls.
-   Register pressure.
-   Window retention.
-   Preallocation/reservation.

Hints express **intent**, not physical implementation.

Software should never need to request a specific physical window such
as:

``` text
window 7
```

Such an interface is intentionally excluded because it would expose
implementation details.

------------------------------------------------------------------------

## 8. Window Exhaustion

The number of resident physical windows is implementation-dependent.

Example implementation:

``` text
W0
W1
W2
W3
W4
W5
W6
W7
```

If execution requires another window while all resident windows are
occupied, the processor automatically selects an eligible older window
and spills its architectural contents to the configured window spill
area.

Conceptually:

``` text
Before:

W0 W1 W2 W3 W4 W5 W6 W7
                         |
                         +-- new call requires another window

After:

W0 contents -> spill memory

W0 is reused as the newly required resident window.
```

When execution later returns to a spilled window, hardware restores its
state before architectural execution resumes.

### Architectural Rule

**Ordinary application software must not need to handle window overflow
or underflow traps during normal operation.**

Implementations may internally use microcode, assists, or other
mechanisms, but transparent spill/restore is the architectural
expectation.

------------------------------------------------------------------------

## 9. Window Spill Area

The operating system/runtime provides a valid memory region for
automatic window spill state.

A privileged architectural register may identify this region:

``` text
WSPBR — Window Spill Base Register
```

Additional implementation-independent state may include:

``` text
WSP     — current window spill position
WDEPTH  — architectural active-window depth/state
```

The exact exposed privileged state should be finalized together with
SeaBird's context-switch and exception architecture.

The operating system must be capable of preserving all architecturally
relevant window state during a task switch.

------------------------------------------------------------------------

## 10. Proposed Instruction and Marker Interface

The following names are **draft mnemonics**. Their semantics are more
important than the final spelling.

### 10.1 Ordinary Windowed Call

``` asm
call foo
```

Default behavior:

-   Performs the procedure call.
-   Requests normal hardware-managed window advancement.
-   Maps outgoing registers to the callee's incoming registers.

Example:

``` asm
mov     r24, r3
mov     r25, r4
call    multiply_values
```

------------------------------------------------------------------------

### 10.2 Ordinary Return

``` asm
ret
```

Default behavior:

-   Returns to the caller.
-   Restores the caller's architectural window relationship.
-   Causes an automatic restore if the caller's window was spilled.

------------------------------------------------------------------------

## 11. Optimization Markers

SeaBird markers may communicate compiler knowledge without changing the
fundamental correctness requirements of the instruction.

### `reuse.call`

Example conceptual syntax:

``` asm
reuse.call foo
```

Meaning:

> The compiler recommends executing the call without allocating a
> completely new window if the implementation can safely do so.

Hardware remains authoritative.

A processor may ignore the hint when reuse is unsafe or disadvantageous.

Possible use:

``` asm
reuse.call tiny_helper
```

------------------------------------------------------------------------

### `assume.reuse call`

If the established SeaBird marker grammar favors a separate marker
prefix:

``` asm
assume.reuse call foo
```

This expresses the same general optimization intent while keeping `call`
itself unchanged.

Only one canonical spelling should survive into the final ISA
specification.

------------------------------------------------------------------------

### `leaf.call`

Conceptual syntax:

``` asm
leaf.call foo
```

Meaning:

> Software believes the destination behaves as a leaf procedure and
> recommends minimizing window-management overhead.

This is an optimization hint, not permission to violate architectural
correctness.

If the assumption cannot be honored safely, hardware may fall back to
ordinary call behavior.

------------------------------------------------------------------------

## 12. Explicit Window Operations

A small number of explicit operations may be useful to runtimes,
compilers, and low-level system software.

### `win.new`

``` asm
win.new
```

Requests advancement to a fresh architectural window without performing
a control-flow transfer.

Possible uses:

-   Runtime-generated call mechanisms.
-   Coroutines.
-   Specialized ABI transitions.
-   Compiler-managed regions.

Hardware automatically spills an older window if necessary.

------------------------------------------------------------------------

### `win.prev`

``` asm
win.prev
```

Requests restoration of the previous architectural window without
performing a normal `ret`.

This operation should be restricted by architectural rules so malformed
software cannot silently corrupt call/window state.

------------------------------------------------------------------------

### `win.reserve`

``` asm
win.reserve
```

Provides a software hint that another window is expected soon.

An implementation may use this to prepare capacity, initiate an early
spill, or otherwise reduce future call latency.

It does **not** guarantee a particular physical window.

------------------------------------------------------------------------

### `win.pin`

``` asm
win.pin
```

Requests temporary retention of the current architectural window.

Potential uses include runtime sequences where premature recycling would
be undesirable.

Whether this operation is unprivileged, restricted, or purely advisory
should be determined during the final ISA review.

------------------------------------------------------------------------

### `win.release`

``` asm
win.release
```

Releases a previous retention request.

------------------------------------------------------------------------

## 13. Example Procedure

Caller:

``` asm
mov     r24, 25
mov     r25, 17
call    calculate
```

Callee:

``` asm
calculate:
    ; Incoming arguments:
    ; R8 = 25
    ; R9 = 17

    add     r16, r8, r9

    ; R16 is local to this window.

    mov     r24, r16
    leaf.call transform

    ret
```

The hardware handles the active window relationships and any required
spills/restores.

------------------------------------------------------------------------

## 14. Example Deep Call Chain

Consider:

``` text
main
  -> A
      -> B
          -> C
              -> D
                  -> E
```

A processor with enough resident windows may retain every active window
directly.

A processor with fewer resident windows may instead perform:

``` text
main window -> spill memory
A
B
C
D
E
```

When the call chain unwinds:

``` text
E -> D -> C -> B -> A -> main
```

the processor restores spilled state as necessary.

Both implementations execute the same SeaBird binary.

------------------------------------------------------------------------

## 15. Relationship to Out-of-Order Register Renaming

Register windowing and OoO register renaming are separate mechanisms.

Windowing determines the current **architectural register namespace**.

Register renaming determines which internal physical storage location
currently represents an architectural register.

An Axium M implementation may therefore perform:

``` text
Instruction references R17
          |
          v
Window controller determines:
"R17 in architectural Window 4"
          |
          v
OoO rename map determines:
"current value is Physical Register P83"
          |
          v
P83
```

Conceptually:

``` text
SeaBird register name
        |
        v
architectural window mapping
        |
        v
OoO rename mapping
        |
        v
physical register
```

The compiler does not need to know the physical rename-register count.

------------------------------------------------------------------------

## 16. Relationship to Superscalar Execution

Superscalar width remains a microarchitectural property.

A one-wide in-order SeaBird processor and a three-wide out-of-order
Axium M processor may implement the same architectural register-window
semantics.

No special window instruction is required merely because a processor is
superscalar.

------------------------------------------------------------------------

## 17. Exceptions and Interrupts

The architecture must guarantee that an exception or interrupt does not
destroy window state.

The final privileged specification must define:

-   Which window is visible to an exception handler.
-   Whether handlers receive a dedicated window or privileged register
    context.
-   How interrupted window depth is recorded.
-   How automatic spills in progress are made precise.
-   How execution resumes into the correct window.
-   How nested interrupts interact with windows.

The implementation may perform additional internal operations as long as
the architectural state appears precise.

------------------------------------------------------------------------

## 18. Context Switching

An operating system must be able to switch processes without knowing the
implementation's physical window count.

Architectural window state must therefore be representable independently
of physical residency.

Conceptually:

``` text
Process A
    |
    +-- active architectural windows
    +-- spilled windows
    +-- window depth/state
    +-- spill-area state

          context switch

Process B
    |
    +-- its own architectural window state
```

The processor and OS cooperate to preserve architectural state, while
physical register allocation remains implementation-specific.

------------------------------------------------------------------------

## 19. Proposed Extension Boundary

The initial SeaBird register-window extension consists conceptually of:

``` text
Automatic:
    CALL            normal windowed call
    RET             normal windowed return
    spill           hardware managed
    restore         hardware managed

Hints / markers:
    reuse.call
    leaf.call
    assume.reuse
    win.reserve

Explicit/runtime:
    win.new
    win.prev
    win.pin
    win.release

Privileged state:
    WSPBR
    WSP
    WDEPTH
```

Final mnemonic names may change as SeaBird's marker grammar and
privileged instruction conventions are standardized.

------------------------------------------------------------------------

## 20. Design Rules

1.  **Correctness must never depend on an optimization marker.**
2.  **Software must not address physical window numbers.**
3.  **The number of resident windows is implementation-specific.**
4.  **Window exhaustion is normally handled transparently by hardware.**
5.  **Outgoing registers become incoming registers across ordinary
    windowed calls.**
6.  **Windowing and OoO register renaming remain architecturally
    distinct.**
7.  **Context switches and precise exceptions must preserve window
    semantics.**
8.  **A binary must remain valid across implementations with different
    physical window counts.**
9.  **Software communicates intent; hardware remains the final authority
    on safe window management.**

------------------------------------------------------------------------

## 21. Initial SeaBird Windowing Profile

``` text
Visible registers:        32
Global registers:         R0-R7
Incoming registers:       R8-R15
Local registers:          R16-R23
Outgoing registers:       R24-R31

Call window advancement:  Automatic
Return restoration:       Automatic
Window overlap:           Supported
Register arguments:       Up to 8 through overlap
Overflow handling:        Automatic spill
Underflow handling:       Automatic restore
Physical window count:    Implementation-specific
Compiler hints:           Supported
Marker integration:       Supported
OoO renaming dependency:  None
OS context preservation:  Required
```

------------------------------------------------------------------------

## 22. Design Principle

SeaBird register windows are intended to provide the performance and
calling-convention advantages of architectural windows without forcing
ordinary software to manually manage the underlying register file.

The extension therefore follows:

> **Hardware manages the mechanism. Software communicates intent.**

This preserves SeaBird's implementation independence while allowing
compilers and runtimes to optimize window behavior when they possess
information that hardware cannot easily infer.
