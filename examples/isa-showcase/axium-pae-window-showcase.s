; Axium M v1: SB32 + PAE32 + register-window ABI showcase.
.text
.cpu axium-m-v1
.globl axium_pae_window_showcase
axium_pae_window_showcase:
  winreserve
  winpin
  winnew
  reuse.call axium_window_leaf
  winprev
  winrelease
  ret

axium_window_leaf:
  ; Outgoing R24-R31 overlap the callee's incoming R8-R15 window.
  add r24, r25
  leaf.call axium_true_leaf
  ret

axium_true_leaf:
  mov r24, r25
  ret

; PAE32 itself changes translation and privileged state, not the ILP32
; instruction encoding. These operations demonstrate the associated MMU/TLB
; management surface while the ELF object records PAE32/window requirements.
.globl axium_pae_system_showcase
axium_pae_system_showcase:
  rdcr r16, 0x120
  wrcr 0x120, r16
  invtlb r16, r17
  invtlbasid r16
  invtlball
  isync
  ret
