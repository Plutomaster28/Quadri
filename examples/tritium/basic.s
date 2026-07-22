# Tritium-v1 embedded integer example.
# Assemble with:
#   llvm-mc -triple=seabird32-unknown-none -mcpu=tritium-v1 \
#     -filetype=obj basic.s -o basic.o

.text
.globl tritium_checksum
.type tritium_checksum,@function
tritium_checksum:
  # R0 is a word pointer and R1 is the element count.
  xor r2, r2
.Lloop:
  jzr r1, .Ldone
  ldw r16, [r0]
  add r2, r16
  inc r0
  inc r0
  inc r0
  inc r0
  dec r1
  jmp .Lloop
.Ldone:
  mov r0, r2
  ret
.size tritium_checksum, .-tritium_checksum
