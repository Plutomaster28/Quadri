.text
.globl seabird_smoke
seabird_smoke:
  mov r0, r1
  add r0, r2
  movi r3, 42
  cmp r0, r3
  je done
  xor r4, r5
done:
  ret
