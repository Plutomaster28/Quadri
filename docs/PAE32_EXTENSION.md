# SeaBird PAE32 Extension

Status: normative for SeaBird architecture 3.2 / SDK 1.0  
Feature name: `PAE32`  
Initial implementation profile: Axium M v1

## Contract

PAE32 gives Tetra-mode systems 36-bit physical addresses while preserving
32-bit virtual addresses, 32-bit pointers, and the SB32 ILP32 ABI. The
architectural physical space is 64 GiB. Axium M v1 may expose at most 48 GiB of
usable RAM; that is a platform limit, not a different PAE format.

PAE32 does not add application instructions. It uses `CR3`, `ASID`, `SFENCE`,
`INVTLB`, `INVTLBASID`, `INVTLBALL`, and the existing SMP shootdown protocol.
QUERY extended-feature bit 13 reports PAE32. In QUERY leaf 7, R0[7:0] reports
the PAE physical width (36), R0[15:8] reports the walk level count (3), and
R2–R3 are reserved. The other leaf-7 fields describe register windows.

## Translation

The virtual address is divided as follows:

| Field | Bits | Entries selected |
|---|---:|---:|
| L1 | 31:30 | 4 |
| L2 | 29:21 | 512 |
| L3 | 20:12 | 512 |
| Offset | 11:0 | 4 KiB page offset |

All entries are aligned, little-endian 64-bit values. The L1 root is 4 KiB
aligned and only entries 0–3 are reachable. An L3 leaf maps 4 KiB. An L2 leaf
with `PS=1` maps an aligned 2 MiB page. `PS` at L1 is reserved.

The original draft's 5/7/8 split was not adopted: an eight-bit L3 index makes
an L2 entry cover 1 MiB, contradicting the required 2 MiB leaf size. Reusing
Tetra's 2/9/9 split preserves existing walkers and gives exact 2 MiB coverage.

## PTE layout

| Bits | Field | Meaning |
|---|---|---|
| 0 | P | Present |
| 1 | W | Write permitted |
| 2 | U | CPL3 access permitted |
| 3 | R | Read permitted |
| 5:4 | MT | WB, WT, UC, or DEVICE memory type |
| 6 | A | Accessed; hardware-set atomically |
| 7 | D | Dirty; hardware-set atomically on leaf write |
| 8 | PS | Legal L2 large-page leaf |
| 9 | G | Global, not tagged by ASID |
| 11:10 | SW | OS-defined |
| 35:12 | PFN | 24-bit physical frame number |
| 51:36 | Reserved | Must be zero |
| 58:52 | SW | OS-defined |
| 62:59 | PKEY | Optional protection key |
| 63 | XD | Execute disable |

The two-bit `MT` encoding is `0=WB`, `1=WT`, `2=UC`, and `3=DEVICE`. No PAE32
encoding selects WC. Page-table pages themselves must use WB memory.

Permissions are intersected across every visited level. Loads require `R`;
stores require `R` and `W`; CPL3 requires `U`; fetch requires `XD=0`.
`CR0.WP` retains its ordinary supervisor-write meaning. Hardware completes
coherent A/D updates before the original access retires.

## Enable and disable sequence

To enable PAE32, privileged software must:

1. Confirm Tetra mode, PAE32 support, and at least 36 PA bits with QUERY.
2. Build valid WB page tables using aligned 64-bit stores.
3. Set `CR1.PAE32`.
4. Write a valid 4 KiB-aligned root and ASID to `CR3`.
5. Set `CR4.PAE_ENABLE`.
6. Set `CR0.PG` with a serializing `WRCR`.

PAE32 cannot be enabled outside Tetra. Clearing `PAE_ENABLE` while paging is
active raises GPF. Changing `CR3`, `PAE_ENABLE`, or `PG` is serializing.

## Faults

`PAGE_FAULT` remains precise and stores the original 32-bit VA in `CR2`.
Source-specific reason values are:

| Reason | Cause |
|---:|---|
| 1 | Not present |
| 2 | Read denied |
| 3 | Write denied |
| 4 | Execute denied |
| 5 | User/supervisor violation |
| 6 | Reserved entry bits |
| 7 | Physical address exceeds 36 bits |
| 8 | Illegal or misaligned large page |

No destination or memory access from the faulting instruction commits. A/D
updates follow the base paging retirement rules.

## TLB and synchronization

Translations are ASID-tagged; global mappings ignore ASID. Software publishes
an entry with one aligned 64-bit store, executes `SFENCE`, invalidates the
affected translations, and executes a second `SFENCE` before recycling a frame
or table. Multiprocessor systems must complete remote shootdown acknowledgement
before reuse.

## Compiler and object behavior

PAE32 does not change pointer representation or ordinary code generation.
Clang targeting `-mcpu=axium-m-v1` defines `__SEABIRD_PAE32__=1` and continues
to use ILP32. Objects that require a PAE32 operating environment set
`EF_SB_PAE32_REQUIRED`; generic SB32 objects remain valid on PAE and non-PAE
systems. The linker ORs the PAE requirement into the output and may therefore
combine generic and PAE-requiring objects; this bit is an environment
requirement, not an ABI identity.
