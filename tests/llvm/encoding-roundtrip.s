.text
.globl encoding_roundtrip
encoding_roundtrip:
  mov r31, r16
  movi r8, 0x1122334455667788
  add r9, r8
  sub r10, r11
  and r12, r13
  or r14, r15
  xor r16, r17
  cmp r18, r19
  je local_target
  jne local_target
  jmp local_target
  call local_target
local_target:
  ret
  push r16
  pop r31
  pusha
  popa
  enter 4294967296
  leave
  pushf
  popf
  pushq r16
  popq r30
  sysret
  getpid r16
  gettid r31
  pdep r16, r17, r31
  pext r31, r16, r17
  lzcnt r31, r16
  tzcnt r31, r16
  popcnt r31, r16
  bextr r31, r16, 132
  binsert r31, r16, 132
  blsi r31, r16
  blsmsk r31, r16
  blsr r31, r16
  rorx r31, r16, 9
  shlx r31, r16, 9
  shrx r31, r16, 9
  andn r31, r16
  bzhi r31, r16
  tzcntv r31, r16
  prefetch [r16 + r17*8 - 32]
  flush [r16 + r17*8 - 32]
  invic [r16 + r17*8 - 32]
  invdc [r16 + r17*8 - 32]
  ldx r31, [r16 + r17*8 - 32]
  stx [r16 + r17*8 - 32], r31
  ldn r31, [r16 + r17*8 - 32]
  stn [r16 + r17*8 - 32], r31
  cpyb r16, r17, r31
  cpyw r16, r17, r31
  memfill r16, r17, r31
  ldp r16, r31, [r17]
  stp [r16 + r17*8 - 32], r31, r30
  fsqrt v31, v16
  fcmp v31, v16
  fneg v31, v16
  fabs v31, v16
  vdiv v31, v16, v30
  vshl v31, v16, 7
  vshr v31, v16, 63
  vdup v31, v16
  vabs v31, v16
  vmax v31, v16, v30
  vmin v31, v16, v30
