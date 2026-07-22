.text
.globl call_external
.type call_external,@function
call_external:
  call external_target
  ret
.size call_external, .-call_external

.data
.globl external_addresses
external_addresses:
  .short external_data
  .long external_data
  .quad external_data
