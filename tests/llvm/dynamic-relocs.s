.text
.globl dynamic_entry
dynamic_entry:
  ret
.globl dynamic_target
dynamic_target:
  ret

.data
.p2align 3
.globl relative_slot
relative_slot:
  .quad 0
  .reloc relative_slot, R_SB_RELATIVE, dynamic_target + 0x1234
.globl global_slot
global_slot:
  .quad 0
  .reloc global_slot, R_SB_GLOB_DAT, dynamic_target
.globl jump_slot
jump_slot:
  .quad 0
  .reloc jump_slot, R_SB_JUMP_SLOT, dynamic_target
