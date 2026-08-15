.text
.globl avx_extended
avx_extended:
  vfmadd v31, v16, v30
  vfmsub v31, v16, v30
  vfnmadd v31, v16, v30
  vfmadd_round v31, v16, v30
  vpermute v31, v16, 255
  vshuffle v31, v16, v30, 255
  vblend v31, v16, v30, 255
  vtest v31, v16
  vpmadd v31, v16, v30
  vreduce_add v31, v16
  vreduce_mul v31, v16
  vcompare_lt v31, v16, v30
  vcompare_gt v31, v16, v30
  vinsert v31, v16, 255
  vextract v31, v16, 255
  vgather v31, [r16 + r17*8 - 32], 255
  vscatter [r16 + r17*8 - 32], v31, 255
  valign v31, v16, v30, 255
  vbswap v31, v16
  vpack v31, v16, v30
  vunpack v31, v16, v30
  vpmul v31, v16, v30
  vperm2 v31, v16, v30, 255
  vcompress v31, v16, 255
  vexpand v31, v16, 255
  vround v31, v16, 255
  vrecip_est v31, v16
  vrsqrt_est v31, v16
  vfmadd_sub v31, v16, v30
  vzeroupper
  vzeroall
  vpmax v31, v16, v30
  vpmin v31, v16, v30
  vgatherq v31, [r16 + r17*8 - 32], 255
  vscatterq [r16 + r17*8 - 32], v31, 255
  vfpclass v31, v16
  vreduce_max v31, v16
  vreduce_min v31, v16
  vmuladdsub v31, v16, v30
  vcompare_eq v31, v16, v30
  vcompare_ne v31, v16, v30
  vcompare_ult v31, v16, v30
  vcompare_ugt v31, v16, v30
  vnot v31, v16
