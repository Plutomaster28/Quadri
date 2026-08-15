.text
.globl performance_markers
performance_markers:
  assume.ld r0, [r1]
  likely.je marker_target
  unlikely.jne marker_target
  stream.st [r1], r0
  prefetch.ld r0, [r1]
  temporary.add r0, r1
  persistent.mov r2, r3
  independent.mul r4, r5
  temporary.fcvt.s2d v0, v1
marker_target:
  ret
