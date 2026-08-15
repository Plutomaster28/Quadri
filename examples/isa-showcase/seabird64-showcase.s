; SeaBird64 ISA showcase.
;
; This file is an assembly/encoding tour, not a program that should execute
; linearly: several routines demonstrate privileged, transactional, or memory
; operations with illustrative operands.  Every major ISA family is present.

.text
.globl seabird64_showcase
seabird64_showcase:
  ; Data movement, integer ALU, carry chains, bit manipulation.
  movi r16, 0x1122334455667788
  mov r17, r16
  add r17, r16
  adc r17, r16
  sub r17, r16
  sbb r17, r16
  mul r17, r16
  umulh r18, r17
  div r17, r16
  and r17, r16
  or r17, r16
  xor r17, r16
  pdep r18, r17, r16
  pext r19, r18, r16
  bextr r20, r19, 132
  binsert r20, r19, 132
  rorx r21, r20, 9
  lzcnt r22, r21
  tzcnt r23, r22
  popcnt r24, r23

  ; Full SIB addressing, pair movement, specialized/string memory, cache.
  lea r25, [r16 + r17*8 - 32]
  ld r26, [r16 + r17*8 - 32]
  st [r16 + r17*8 - 32], r26
  ldp r26, r27, [r16]
  stp [r16 + r17*8 - 32], r26, r27
  cpyb r16, r17, r18
  cpyw r16, r17, r18
  memfill r16, r17, r18
  prefetch [r16 + r17*8 - 32]
  flush [r16 + r17*8 - 32]
  invic [r16 + r17*8 - 32]
  invdc [r16 + r17*8 - 32]

  ; Synchronization and atomic read-modify-write.
  fence
  lfence
  sfence
  mfence
  ll r26, [r16]
  sc r26, [r16]
  atadd r26, [r16]
  cmpxchg r26, r27, [r16]

  ; Control flow and architecturally inert performance markers.
  cmp r16, r17
  likely.je .Llikely
  unlikely.jne .Lunlikely
.Llikely:
  temporary.add r16, r17
  jmp .Ljoin
.Lunlikely:
  persistent.mov r16, r17
.Ljoin:
  independent.mul r16, r17
  reuse.call .Lleaf
  leaf.call .Lleaf
  ret
.Lleaf:
  ret

.globl seabird64_fp_simd_showcase
seabird64_fp_simd_showcase:
  ; Scalar IEEE binary32/binary64/binary128 and conversions.
  fld v0, [r16]
  fst [r16], v0
  fadd v0, v1, v2
  fsub v0, v1, v2
  fmul v0, v1, v2
  fdiv v0, v1, v2
  fsqrt v2, v0
  fmadd v3, v0, v1, v2
  fmin v4, v0, v1
  fmax v5, v0, v1
  fcmp v0, v1
  fcvt.s2d v6, v0
  fcvt.d2s v7, v6
  fclass v8, v7
  fadd.s v9, v10, v11
  fsqrt.s v11, v9
  fadd.q v12, v13, v14
  fsqrt.q v14, v12
  fcvtu v15, r16
  fcvtus r17, v15

  ; Base SIMD plus advanced vector arithmetic, masks, memory, and reduction.
  vadd v16, v17, v18
  vsub v16, v17, v18
  vmul v16, v17, v18
  vdiv v16, v17, v18
  vand v16, v17, v18
  vor v16, v17, v18
  vxor v16, v17, v18
  vnot v16, v17
  vshl v16, v17, 7
  vmax v16, v17, v18
  vmin v16, v17, v18
  vcompare_eq v19, v17, v18
  vcompare_ne v20, v17, v18
  vcompare_ult v21, v17, v18
  vcompare_ugt v22, v17, v18
  vfmadd v23, v17, v18
  vpermute v24, v23, 255
  vshuffle v25, v23, v24, 255
  vblend v26, v24, v25, 255
  vgather v27, [r16 + r17*8 - 32], 255
  vscatter [r16 + r17*8 - 32], v27, 255
  vgatherq v28, [r16 + r17*8 - 32], 255
  vscatterq [r16 + r17*8 - 32], v28, 255
  vreduce_add v29, v28
  vreduce_max v30, v28
  vcompress v31, v30, 255
  vexpand v31, v30, 255
  vzeroupper
  vzeroall
  ret

.globl seabird64_crypto_dsp_showcase
seabird64_crypto_dsp_showcase:
  ; AES, SHA, carry-less/Galois-field and polynomial operations.
  aesenc v0, v1, v2
  aesdec v0, v1, v2
  aesimc v3, v0
  pclmulqdq v4, v1, v2
  ghash v5, v1, v2
  sha1_msg1 v6, v1
  sha1_msg2 v7, v6
  sha256_sig0 v8, v1
  sha256_sig1 v9, v8
  poly_mul v10, v1, v2

  ; Scalar/fixed-point/complex/vector DSP.
  mac32 r16, r17, r18
  mac64 r16, r17, r18
  satadd r19, r17, r18
  satsub r20, r17, r18
  fixed_mul r21, r17, r18, r19
  cmplx_mul r22, r17, r18, r19
  bitrev r23, r22, 64
  pack_sat r24, r22, r23
  clamp r25, r24, r17, r18
  dotp r26, v1, v2
  sumdotp r27, v2
  rshift_round r28, r27, 7
  mad32 r29, r17, r18, r19
  ret

.globl seabird64_transaction_system_showcase
seabird64_transaction_system_showcase:
  ; Transactional memory. The fallback makes this block structurally valid.
  xbegin .Ltxn_fallback
  xend
  jmp .Ltxn_done
.Ltxn_fallback:
  xstatus r16
  xtest r17
  xabort 42
.Ltxn_done:

  ; Privileged/system/VM/security examples: assemble and inspect, do not call
  ; this routine from an unprivileged demo program.
  rdtime r18
  rdts r19
  query r20, r21
  rngget r22
  rdcr r23, 0x100
  wrcr 0x100, r23
  invtlbasid r24
  invtlball
  sendipi r24, r25, r26
  xsave [r16], r27
  xrstor [r16], r27
  vmenter [r16]
  vmresume [r16]
  vmread r28, 1
  vmwrite 1, r28
  endbr
  wrss [r16], r29
  rdpmc r30, 0
  isync
  ret

.section .rodata,"a",@progbits
.p2align 4
.globl seabird64_showcase_message
seabird64_showcase_message:
  .asciz "SeaBird64 ISA showcase"
  .quad 0x1122334455667788

.data
.p2align 3
.globl seabird64_showcase_pointer
seabird64_showcase_pointer:
  .quad seabird64_showcase_message

.bss
.p2align 4
.globl seabird64_showcase_workspace
seabird64_showcase_workspace:
  .zero 256
