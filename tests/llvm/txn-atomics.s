.text
.globl txn_atomics
txn_atomics:
  xbegin txn_fallback
  xbegina r31
  xend
  xabort 255
  xabort r31
  xtest r31
  xstatus r31
  xchg r31, [r16 + r17*8 - 32]
  xchg128 v31, [r16 + r17*8 - 32]
txn_fallback:
  ret
