.text
.globl dsp_extended
dsp_extended:
  mac32 r31, r16, r30
  mac64 r31, r16, r30
  macs r31, r16, r30
  msub r31, r16, r30
  satsub r31, r16, r30
  satadd r31, r16, r30
  fixed_mul r31, r16, r30, r29
  fixed_add r31, r16, r30
  cmplx_mul r31, r16, r30, r29
  bitrev r31, r16, 64
  pack_sat r31, r16, r30
  unpack_exp r31, r16
  clamp r31, r16, r30, r29
  accumulate r31, r16
  dotp r31, v16, v30
  sumdotp r31, v16
  rshift_round r31, r16, 63
  lshift r31, r16, 31
  sllv r31, r16, r30
  srlv r31, r16, r30
  srav r31, r16, r30
  rndq r31, r16, r30
  clz_fast r31, r16
  tzcnt_fast r31, r16
  mad32 r31, r16, r30, r29
