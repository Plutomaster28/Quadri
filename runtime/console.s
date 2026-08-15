.text

.globl sb_read_int
.type sb_read_int,@function
sb_read_int:
  movi r0, 1
  syscall
  ret
.size sb_read_int, .-sb_read_int

.globl sb_print_int
.type sb_print_int,@function
sb_print_int:
  mov r1, r0
  movi r0, 2
  syscall
  ret
.size sb_print_int, .-sb_print_int

.globl sb_print_char
.type sb_print_char,@function
sb_print_char:
  mov r1, r0
  movi r0, 3
  syscall
  ret
.size sb_print_char, .-sb_print_char

.globl sb_print_string
.type sb_print_string,@function
sb_print_string:
  mov r1, r0
  movi r0, 4
  syscall
  ret
.size sb_print_string, .-sb_print_string

.globl sb_read_char
.type sb_read_char,@function
sb_read_char:
  movi r0, 5
  syscall
  ret
.size sb_read_char, .-sb_read_char
