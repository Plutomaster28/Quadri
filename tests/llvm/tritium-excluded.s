ldq r0, [r0]
fadd v0, v1, v2
fsqrt v0, v1
fmadd v0, v1, v2, v3
aesenc v0, v1, v2
mac32 r0, r1, r2
xbegin 0
xchg r0, [r1]
xchg128 v0, [r1]
vadd v0, v1, v2
vdiv v0, v1, v2
vfmadd v0, v1, v2
isync
setmode 3
syscall
pusha
ldp r0, r1, [r2]
pdep r0, r1, r2
