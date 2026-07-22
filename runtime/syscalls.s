.text
.globl sb_host_value
.type sb_host_value,@function
sb_host_value:
  movi r0, 2
  syscall
  ret
.size sb_host_value, .-sb_host_value
