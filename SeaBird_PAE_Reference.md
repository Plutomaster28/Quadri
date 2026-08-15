# SeaBird PAE Extension — Superseded Reference Draft

> **Non-normative and superseded.** This proposal is retained for design
> provenance. Its 5/7/8 walk and conceptual MMU registers were rejected during
> ratification. Implementations must use `docs/PAE32_EXTENSION.md`,
> `spec/architectural-layouts.json`, and the generated ISA volumes.

## 1. Purpose

The SeaBird Physical Address Extension (PAE) extends a 32-bit SeaBird
implementation beyond the normal 4 GiB physical-address limit without
changing the ISA's 32-bit virtual-address model.

The extension is intentionally conservative: it expands system memory
capability while keeping the programming model and base ISA simple.

## 2. Architectural Summary

  -----------------------------------------------------------------------
  Property                            Definition
  ----------------------------------- -----------------------------------
  Virtual address width               32 bits

  Physical address width              36 bits

  Initial Axium M usable-memory       Up to 48 GiB
  target                              

  Architectural physical-address      64 GiB addressable by 36 bits
  capacity                            

  Base page size                      4 KiB

  Optional large page size            2 MiB

  Page-table entry width              64 bits

  Page-table hierarchy                3 levels

  Memory permissions                  Read / Write / Execute plus User /
                                      Supervisor

  Page-state tracking                 Present, Accessed, Dirty

  Address-space identification        ASID supported

  TLB management                      Architecturally defined
                                      invalidation operations

  PAE control                         Privileged MMU state
  -----------------------------------------------------------------------

### 2.1 48 GiB vs. 36-bit Addressing

SeaBird PAE defines a **36-bit physical-address architecture**, which is
capable of representing 64 GiB of physical address space.

The first Axium M implementation may limit installed or usable physical
memory to **48 GiB**. The 48 GiB value is therefore an
implementation/platform limit rather than a permanent ISA limit. Future
SeaBird processors may use more of the 36-bit physical address space
without changing the PAE architecture.

## 3. Addressing Model

PAE does **not** enlarge a process's virtual address space.

A SeaBird application continues to use 32-bit virtual addresses:

``` text
32-bit virtual address
        |
        v
   PAE page tables
        |
        v
36-bit physical address
```

Each process therefore retains a maximum 4 GiB virtual address space,
while the operating system may map pages belonging to different
processes anywhere within the larger physical address space.

Example:

``` text
Process A VA 0x10000000 -> PA 0x1_10000000
Process B VA 0x10000000 -> PA 0x7_10000000
Process C VA 0x10000000 -> PA 0xB_10000000
```

## 4. Base Page Size

The standard SeaBird PAE page size is **4 KiB**.

A 4 KiB page requires a 12-bit page offset, leaving 20 bits of a 32-bit
virtual address for page-table indexing.

``` text
31                              12 11              0
+--------------------------------+------------------+
|      Virtual Page Number       |   Page Offset    |
|            20 bits             |     12 bits      |
+--------------------------------+------------------+
```

An optional **2 MiB large-page mode** may also be provided.

## 5. Page-Table Hierarchy

SeaBird PAE uses a three-level hierarchy.

The initial index allocation is:

``` text
31       27 26        20 19        12 11          0
+----------+------------+------------+--------------+
|    L1    |     L2     |     L3     |    Offset    |
|  5 bits  |   7 bits   |   8 bits   |   12 bits    |
+----------+------------+------------+--------------+
```

This provides:

-   32 possible L1 entries.
-   128 possible L2 entries per L2 table.
-   256 possible L3 entries per L3 table.
-   4 KiB pages.
-   Coverage of the complete 32-bit virtual-address space.

## 6. Page-Table Entries

All PAE page-table entries are **64 bits wide**.

The entry contains a physical page reference and architectural
attributes.

Conceptual layout:

``` text
63                                              12 11                0
+------------------------------------------------+-------------------+
|       Physical Page Number / Reserved          |    Attributes     |
+------------------------------------------------+-------------------+
```

### 6.1 Initial Attribute Definition

  ------------------------------------------------------------------------
                           Bit Name                  Purpose
  ---------------------------- --------------------- ---------------------
                             0 `PRESENT`             Entry/page is valid
                                                     and present

                             1 `WRITE`               Writes are permitted

                             2 `USER`                User-mode access is
                                                     permitted

                             3 `EXECUTE`             Instruction execution
                                                     is permitted

                             4 `ACCESSED`            Page has been
                                                     accessed

                             5 `DIRTY`               Page has been written

                             6 `GLOBAL`              Mapping may remain
                                                     across address-space
                                                     changes

                             7 `CACHE_DISABLE`       Disables normal
                                                     caching for the
                                                     mapping

                             8 `LARGE_PAGE`          Entry represents a
                                                     supported large-page
                                                     mapping

                         9--11 Reserved / software   Reserved for future
                                                     architecture or
                                                     OS-defined use

                           12+ Physical page number  Physical page address
                                                     information
  ------------------------------------------------------------------------

Reserved architectural bits must have defined behavior in the final
specification before software relies upon them.

## 7. Memory Protection

SeaBird PAE provides independent page permissions for:

-   Read access
-   Write access
-   Execute access
-   User-mode access
-   Supervisor-mode access

The architecture should therefore support mappings such as:

``` text
R--
RW-
R-X
RWX
```

Explicit execute permission allows an operating system to implement
non-executable data pages and W\^X-style memory policies.

## 8. MMU Architectural State

PAE is controlled through privileged MMU state rather than a large
family of dedicated PAE instructions.

### `MMUCTL`

The MMU control register contains global virtual-memory configuration.

Initial conceptual fields:

``` text
bit 0    VM_ENABLE
bit 1    PAE_ENABLE
bit 2    NX_ENABLE
bit 3    LARGE_PAGE_ENABLE
...
```

Exact bit positions beyond the initial definition remain reserved until
the complete privileged SeaBird architecture is finalized.

### `PTBR`

**Page Table Base Register**

Contains the physical location of the root PAE page table.

Because physical addresses can exceed 32 bits, the architectural
definition of `PTBR` must be capable of representing the required 36-bit
physical address.

### `ASID`

**Address Space Identifier**

Identifies the currently active virtual address space. ASIDs allow TLB
entries belonging to different processes/address spaces to coexist
without requiring a complete TLB flush on every context switch.

## 9. TLB Architecture

A SeaBird PAE implementation may use any internal TLB organization,
capacity, associativity, or replacement policy.

Software-visible behavior, however, is architectural.

SeaBird must provide mechanisms equivalent to:

``` asm
tlbinv      address
tlbinv.all
```

The final privileged ISA may generalize these names or combine them with
existing SeaBird system-management operations.

The architecture must define:

-   Invalidation of one virtual mapping.
-   Invalidation of all relevant mappings.
-   Interaction with ASIDs.
-   Treatment of global mappings.
-   Required synchronization after page-table modification.

## 10. Page Faults

PAE must provide precise faults for invalid or prohibited translations.

At minimum, the processor must distinguish causes equivalent to:

-   Page not present.
-   Read protection violation.
-   Write protection violation.
-   Execute protection violation.
-   User/supervisor protection violation.
-   Invalid/reserved page-table entry.
-   Invalid physical address.
-   Unsupported large-page mapping.

The fault mechanism should expose enough information for the operating
system to identify the faulting virtual address and reason for the
fault.

## 11. PAE Disabled

When PAE is disabled, a 32-bit SeaBird implementation may use its normal
32-bit physical-address mechanism.

``` text
PAE disabled:
32-bit VA -> normal translation -> 32-bit PA
```

When PAE is enabled:

``` text
PAE enabled:
32-bit VA -> PAE translation -> 36-bit PA
```

PAE therefore extends physical addressing without changing ordinary
32-bit pointers or requiring applications to use a new pointer
representation.

## 12. Implementation Independence

The following properties are **not** specified by the PAE ISA extension
and remain implementation-specific:

-   TLB capacity
-   TLB associativity
-   TLB replacement algorithm
-   Cache sizes
-   Cache hierarchy
-   Pipeline design
-   In-order or out-of-order execution
-   Superscalar width
-   Register renaming
-   Memory-controller implementation
-   Maximum installed RAM below the architectural address limit

This allows simple and high-performance SeaBird processors to implement
the same PAE architecture differently.

## 13. Initial Axium M Profile

The first Axium M PAE implementation is presently defined around:

``` text
ISA:                 SeaBird 32-bit
Virtual addressing:  32-bit
Physical addressing: 36-bit
Usable RAM target:   <= 48 GiB
Base pages:          4 KiB
Large pages:         2 MiB optional
PTE width:           64-bit
Page-table levels:   3
ASIDs:                Supported
R/W/X protection:    Supported
TLB management:      Supported
```

## 14. Design Principle

SeaBird PAE is intended to be an **extension of capability, not a
redesign of the base ISA**.

The extension should expose only the architectural mechanisms that
operating-system software requires. Microarchitectural details remain
invisible to software wherever possible.

This preserves the ability for multiple processors---from simple
in-order SeaBird cores to superscalar out-of-order Axium
implementations---to share the same software architecture.
