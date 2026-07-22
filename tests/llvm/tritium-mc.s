.text
.globl tritium_mc_smoke
tritium_mc_smoke:
  mov r31, r16
  movzx r0, r1
  movsx r0, r1
  movhi r0, r1
  movlo r0, r1
  movswp r0, r1
  ldi r16, 305419896
  ld r0, [r1 + r2*4 + 16]
  st [r1 + r2*4 + 16], r0
  leas r31, [r16 + r17*8 - 32]
  xchg r16, r31
  add r0, r1
  ldw r2, [r3]
  stw [r4 + 4], r5
  cmp r0, r2
  slt r0, r2
  mul r0, r2
  mulh r0, r2
  neg r0
  inc r16
  dec r31
  not r0
  abs r0, r2
  clz r0, r2
  ctz r0, r2
  popc r0, r2
  addi r0, 123456
  subi r16, -17
  muli r31, 9
  divi r0, -3
  modi r0, 7
  cmpi r0, -1
  cmps r0, r2
  cmpu r0, r2
  tst r0, r2
  tsti r0, 255
  div r0, r2
  mod r0, r2
  umul r0, r2
  udiv r0, r2
  adds r0, r2
  addu r0, r2
  subs r0, r2
  subu r0, r2
  nand r0, r2
  nor r0, r2
  xnor r0, r2
  rol r0, r2
  ror r0, r2
  bset r0, r2
  bclr r16, r31
  btog r31, r16
  btst r0, r2
  mask r0, r2, 255
  ext r31, r16, 132
  max r0, r2
  min r0, r2
  sgt r0, r2
  jo tritium_done
  jno tritium_done
  js tritium_done
  jns tritium_done
  jzr r16, tritium_done
  jnzr r31, tritium_done
  jmpa r16
  brr r31
  trap 1
  yield
  push r16
  pop r31
  enter 32
  leave
  pushf
  popf
  hlt
  reset
  rdcr r16, 4660
  wrcr 4660, r31
  iret
  cli
  sti
  wfi
  rdtime r16
  rdts r31
  sleep 1000
  query r16, r31
  eoi r16
  savectx [r16 + r17*8 - 32]
  loadctx [r16 + r17*8 - 32]
  getcpl r31
  cmpxchg r16, r31, [r17 + r18*4 - 16]
  atadd r16, [r17 + 4]
  atsub r16, [r17 + 4]
  atand r16, [r17 + 4]
  ator r16, [r17 + 4]
  atxor r16, [r17 + 4]
  ll r16, [r17 + 4]
  sc r16, [r17 + 4]
  fence
  lfence
  sfence
  mfence
  jne tritium_done
tritium_done:
  ret
