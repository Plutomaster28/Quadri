define double @fp_constant() {
entry:
  ret double 1.500000e+00
}

define double @fp_add_constant(double %value) {
entry:
  %result = fadd double %value, 2.500000e+00
  ret double %result
}
