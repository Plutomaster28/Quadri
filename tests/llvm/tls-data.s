.text
.globl tls_entry
tls_entry:
  ret

.section .tdata,"awT",@progbits
.p2align 3
.globl tls_value
tls_value:
  .quad 7

.section .tbss,"awT",@nobits
.p2align 3
.globl tls_zero
tls_zero:
  .zero 8

.data
.p2align 3
.globl tls_local_offset
tls_local_offset:
  .quad 0
  .reloc tls_local_offset, R_SB_TLS_LE, tls_value
