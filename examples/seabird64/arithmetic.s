# SeaBird64 arithmetic and function-call example.
# Assemble with:
#   llvm-mc -triple=seabird64-unknown-none -filetype=obj arithmetic.s -o arithmetic.o

.text
.globl seabird_add_then_scale
.type seabird_add_then_scale,@function
seabird_add_then_scale:
  # R0 and R1 contain the first two integer arguments; R0 is the return value.
  add r0, r1
  movi r2, 4
  mul r0, r2
  ret
.size seabird_add_then_scale, .-seabird_add_then_scale

.globl seabird_example_entry
.type seabird_example_entry,@function
seabird_example_entry:
  movi r0, 10
  movi r1, 5
  call seabird_add_then_scale
  ret
.size seabird_example_entry, .-seabird_example_entry
