# SeaBird64 scalar floating-point spellings.
# Unsuffixed operations are binary64, .s is binary32, and .q is binary128.

.text
.globl seabird_add_f64
.type seabird_add_f64,@function
seabird_add_f64:
  fadd v0, v0, v1
  ret
.size seabird_add_f64, .-seabird_add_f64

.globl seabird_add_f32
.type seabird_add_f32,@function
seabird_add_f32:
  fadd.s v0, v0, v1
  ret
.size seabird_add_f32, .-seabird_add_f32

.globl seabird_add_f128
.type seabird_add_f128,@function
seabird_add_f128:
  fadd.q v0, v0, v1
  ret
.size seabird_add_f128, .-seabird_add_f128
