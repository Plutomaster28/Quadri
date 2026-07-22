.text
.globl section_entry
section_entry:
  ret

.section .rodata,"a",@progbits
.p2align 3
.globl section_constant
section_constant:
  .quad 0x1122334455667788

.data
.p2align 3
.globl section_pointer
section_pointer:
  .quad section_constant

.bss
.p2align 4
.globl section_zeros
section_zeros:
  .zero 16
