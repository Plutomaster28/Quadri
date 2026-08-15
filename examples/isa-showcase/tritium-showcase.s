; Tritium v1 constrained SB32/ILP32 showcase.
.text
.cpu tritium-v1
.globl tritium_showcase
tritium_showcase:
  ldi r16, 0x12345678
  mov r17, r16
  add r17, r16
  adds r17, r16
  subs r17, r16
  umul r17, r16
  udiv r17, r16
  nand r18, r17
  rol r18, r17
  bset r18, r17
  mask r19, r18, 255
  ext r20, r19, 132
  ldw r21, [r16 + r17*4 + 16]
  stw [r16 + r17*4 + 16], r21
  cmpxchg r21, r22, [r16]
  atadd r21, [r16]
  fence
  cmp r17, r18
  likely.jne .Ltritium_branch
  trap 1
.Ltritium_branch:
  rdtime r23
  query r24, r25
  ret
