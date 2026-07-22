.text
.globl fpx_extended
fpx_extended:
  fmadd v31, v16, v30, v29
  fmsub v31, v16, v30, v29
  fnmadd v31, v16, v30, v29
  fnmsub v31, v16, v30, v29
  fmin v31, v16, v30
  fmax v31, v16, v30
  frecip v31, v16
  frsqrt v31, v16
  frnd v31, v16
  frndz v31, v16
  fcvt.s2d v31, v16
  fcvt.d2s v31, v16
  fcvtint v31, v16
  fclass v31, v16
  fchs v31, v16
  ftest v31, v16
  fld v31, [r16 + r17*8 - 32]
  fst [r16 + r17*8 - 32], v31
