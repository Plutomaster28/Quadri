.text
.globl sysx_extended
sysx_extended:
  in r31, 65535
  out 65535, r31
  xsave [r16 + r17*8 - 32], r30
  xrstor [r16 + r17*8 - 32], r30
  isync
  invtlb r31, r16
  invtlbasid r31
  invtlball
  sendipi r31, r16, r30
  vmenter [r16 + r17*8 - 32]
  vmresume [r16 + r17*8 - 32]
  vmread r31, 65535
  vmwrite 65535, r31
  endbr
  wrss [r16 + r17*8 - 32], r30
  rdpmc r31, 65535
  rngget r31
  setmode 3
