.text
.cpu axium-m-v1
.globl windowing_ops
windowing_ops:
  winnew
  winreserve
  winpin
  winrelease
  winprev
  reuse.call window_target
  leaf.call window_target
window_target:
  ret
