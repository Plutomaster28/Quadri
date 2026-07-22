.file "assembler-directives.s"
.arch seabird
.cpu seabird-gold
.mode DRAGONET
.text
.globl directive_entry
.type directive_entry,@function
.p2align 4
directive_entry:
  ret
.size directive_entry, .-directive_entry

.section .rodata,"a",@progbits
.equ directive_constant, 0x1234
.set directive_mutable, 7
.set directive_mutable, 8
.globl directive_data
.type directive_data,@object
.p2align 3
directive_data:
  .byte 0x11
  .word 0x2233
  .dword 0x44556677
  .qword 0x8899aabbccddeeff
  .float 1.5
  .double 2.5
  .ascii "Sea"
  .asciz "Bird"
  .string "ISA"
  .space 3
.size directive_data, .-directive_data

.section .data,"aw",@progbits
.globl directive_pointer
directive_pointer:
  .qword directive_data

.section .bss,"aw",@nobits
.p2align 4
.globl directive_zeros
directive_zeros:
  .zero 16

.message "SeaBird assembler directive test"
.warning "SeaBird assembler warning test"
