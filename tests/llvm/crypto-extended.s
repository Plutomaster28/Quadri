.text
.globl crypto_extended
crypto_extended:
  aesenc v31, v16, v30
  aesdec v31, v16, v30
  aesimc v31, v16
  pclmulqdq v31, v16, v30
  ghash v31, v16, v30
  sha1_msg1 v31, v16
  sha1_msg2 v31, v16
  sha256_sig0 v31, v16
  sha256_sig1 v31, v16
  poly_mul v31, v16, v30
