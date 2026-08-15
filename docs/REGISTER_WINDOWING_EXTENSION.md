# SeaBird Register-Windowing Extension

Status: normative for SeaBird architecture 3.2 / SDK 1.0  
Feature name: `REGISTER_WINDOWS` / LLVM `+register-windows`  
Initial implementation profile: Axium M v1

## Compatibility boundary

Register windowing is an optional ABI and execution-context mode. It is active
only when QUERY reports the feature, `CR1.REGISTER_WINDOWS` and
`CR4.WINDOW_ENABLE` are set, and the executable is marked
`EF_SB_WINDOWED_ABI`. With the control clear, `CALL`, `CALLA`, and `RET` retain
the ordinary SeaBird ABI exactly.

A linker must reject direct mixing of windowed and ordinary objects. Explicit
ABI thunks may bridge them by moving arguments/results and changing window mode
only through the privileged runtime contract described below. An ordinary
application call cannot change ABI mode.

## Register partition

| Registers | Role | Window behavior |
|---|---|---|
| R0–R7 | Global/shared | Do not change identity; R7 remains SP |
| R8–R15 | Incoming | Alias the caller's outgoing registers |
| R16–R23 | Local | Private to the current logical window |
| R24–R31 | Outgoing | Become the child's incoming registers |

V0–V31 and K0–K7 are not windowed. Their ordinary caller/callee rules remain.

The windowed ABI passes up to eight integer/pointer arguments in caller
R24–R31. The callee observes them as R8–R15. Callee integer returns use R8/R9;
after `RET`, the caller observes them in R24/R25. Excess and variadic arguments
use the ordinary stack layout. Floating/vector arguments and returns retain
V0–V7 and V0/V1.

## Logical operations

When enabled, `CALL` and `CALLA` first record the return address using the
ordinary stack/shadow-stack contract, then advance one logical window before
the destination executes. `RET` validates its ordinary and shadow return
state, restores the parent logical window, and transfers control atomically.
`CALLA` latches its target before advancing, so the target may reside in any
caller-visible register, including an outgoing register.

The logical window stack is architectural. The number of resident physical
windows, backing registers, spill selection, caching, and microcode are not.
Implementations with different resident counts execute the same binary.

The reset/current root window consists of the already-visible R8–R31 values.
Creating a child aliases caller R24–R31 to child R8–R15 and initializes the
child's R16–R31 to zero. This zeroing is architectural even when physical
backing is reused. Returning restores the parent and propagates all child
R8–R15 values back to parent R24–R31; the ABI designates only the first two as
integer results.

## Window instructions

All instructions use SYSX group 04 and require the feature:

| Instruction | Sub-opcode | Effect |
|---|---:|---|
| `WINNEW` | 1B | Advance a logical window without control transfer |
| `WINPREV` | 1C | Restore the parent without returning |
| `WINRESERVE` | 1D | Advisory request to prepare microarchitectural resident capacity |
| `WINPIN` | 1E | Increment the current advisory retention count |
| `WINRELEASE` | 1F | Decrement the retention count |

`WINPREV` at the root and unbalanced `WINRELEASE` raise GPF. `WINRESERVE` does
not spill, fault, or change architectural state. Pinning cannot prevent a spill required for
correctness or forward progress.

## Call markers

Marker ID 9 is `reuse.call`; marker ID 10 is `leaf.call`. Both preserve the
logical CALL transition. `reuse` permits physical backing reuse only when the
implementation proves isolation remains exact. `leaf` reports the compiler's
expectation that the target makes no windowed call. Either hint may be ignored;
an incorrect hint cannot corrupt registers or suppress faults.

## Spill area and state

The privileged state is:

| Register | Definition |
|---|---|
| WSPBR | 64-byte-aligned spill-area base |
| WSP | Current byte position |
| WDEPTH | Read-only logical depth and spilled count |
| WSTATUS | Area limit, pin count, and status |

All four registers are 64 bits. Their exact layouts are:

| Register | Bits | Field |
|---|---:|---|
| WSPBR | 63:6 | `BASE_63_6`; bits 5:0 are zero |
| WSP | 63:6 | `NEXT_FREE_OFFSET_63_6`; bits 5:0 are zero |
| WDEPTH | 31:0 | `ACTIVE_DEPTH`, where the root is zero |
| WDEPTH | 63:32 | `SPILLED_WINDOWS` |
| WSTATUS | 31:0 | Writable exclusive limit in 64-byte units |
| WSTATUS | 47:32 | Read-only current `PIN_COUNT` |
| WSTATUS | 48 | Read-only `ASSIST_PENDING` |
| WSTATUS | 49 | Read-only `AREA_EXHAUSTED` |
| WSTATUS | 50 | Read-only `AT_ROOT` |

Unlisted bits are reserved and read zero. `WSPBR` is a virtual address,
zero-extended to the active address width. `WSP` is a byte offset relative to
that base. WSPBR, WSP, and the WSTATUS limit may be written only while
windowing is disabled and `ACTIVE_DEPTH=0`; otherwise WRCR raises GPF without
changing state. Writes cannot modify read-only fields.

Reset clears WSPBR, WSP, and the limit, establishes the root at depth zero,
and clears spilled count, pins, and pending/exhausted state. `AT_ROOT` reads
one. Before enabling windowing, privileged software installs an aligned base,
a nonzero limit, and WSP=0. Setting `WINDOW_ENABLE` requires QUERY and
`CR1.REGISTER_WINDOWS`, valid spill configuration, depth zero, no pins, and no
pending assist. Clearing either enable while away from the root, pinned, or
assisting raises GPF and leaves the control unchanged.

The implementation validates the complete target record before committing a
spill or restore. Alignment, translation, or permission failure raises a
precise fault with the logical window unchanged. Spill accesses obey ordinary
core-context memory ordering and complete before the triggering instruction
retires. Register payloads are 48, 96, 192, and 384 bytes in Clownfish, Tetra,
Dragonet, and Droplet respectively. Their 64-byte-aligned record strides are
64, 128, 192, and 384 bytes; the area and each record are 64-byte aligned.

A record contains R8 through R31 in increasing register-number order. Each
value occupies the active pointer width and is little-endian; padding through
the stride is zero. WSP names the next free record: a spill writes the complete
record at `WSPBR+WSP` and then advances WSP by the stride; a restore first
decrements WSP and then reads that record. Validation precedes either update.

If no spillable capacity exists or the bounded area is exhausted, the
triggering operation raises GPF before changing the call stack or window state.
Normal correctly provisioned application execution does not receive
overflow/underflow traps.

## Exceptions, debugging, and context switching

An exception enters a dedicated privileged handler context; it does not consume
an application window. The frame records the interrupted logical-window token
and whether a transparent spill/restore was pending. A pending assist either
commits completely before delivery or is discarded. `IRET` validates and
restores the token atomically. Nested handlers use the privileged handler stack.

Debuggers identify registers by logical depth plus architectural register
number, never by physical window number. XSAVE component 6 contains the
64-byte `WINDOW_STATE_HEADER` followed by the logical/spilled window image.
`SAVECTX` first materializes every non-current logical window into the
task-owned spill area, then records the window header, bounds, and current
token in `CORE_CONTEXT.system_state`; the current visible registers remain in
the ordinary GPR fields. `LOADCTX` validates that header and every referenced
spill record before changing state. XSAVE component 6 is the portable
self-contained snapshot form. XRSTOR validates the entire component before
changing any register.

`WINDOW_STATE_HEADER` uses magic bytes `SBWN`, revision 1, and header size 64.
Its fields are active depth, spilled count, spill position, spill limit,
current pin count, flags, an opaque current token, record stride, mode, and
reserved bytes in the offsets specified by Volume 5. XSAVE writes `depth+1`
records after the header in root-to-current order, including the current
window. Flag bit 0 means the image is complete; bit 1 means the current record
is present. All other flag and reserved bits are zero.

## ABI transition thunk

Only a CPL0 runtime may bridge ABI modes. It must enter with windowing disabled
and at ordinary ABI depth, marshal ordinary arguments into a newly initialized
root's outgoing registers, install the task spill state, enable windowing, and
call the windowed destination. On return at the root it copies results back,
disables windowing, and restores the ordinary context. The reverse direction
first reaches the window root, materializes the window context, disables the
mode, calls the ordinary target, then reconstructs and validates the saved
window context before re-enabling. Interrupts use the dedicated handler context
throughout. A thunk may not toggle either enable with a live child window.

## Compiler behavior

LLVM CPU `axium-m-v1` enables PAE32 and register windows for SB32. Its calling
convention lowers caller arguments to R24–R31, formal arguments to R8–R15,
callee results to R8/R9, and caller results to R24/R25. R8–R23 are preserved
across a logical call by window restoration without software spills; global R6
retains its frame-pointer preservation rule. Variadic unnamed arguments remain
stack-only.

Clang defines `__SEABIRD_REGISTER_WINDOWS__=1` for that CPU. The assembler
accepts window operations only with the feature enabled and accepts
`reuse.call`/`leaf.call` only on calls. Automatic marker emission is deliberately
conservative; frontends may emit these hints only with reliable interprocedural
knowledge.
