target triple = "seabird64-unknown-none"

declare double @llvm.sqrt.f64(double)
declare double @llvm.fabs.f64(double)

define double @seabird_fsqrt(double %value) {
entry:
  %result = call double @llvm.sqrt.f64(double %value)
  ret double %result
}

define double @seabird_fneg(double %value) {
entry:
  %result = fneg double %value
  ret double %result
}

define double @seabird_fabs(double %value) {
entry:
  %result = call double @llvm.fabs.f64(double %value)
  ret double %result
}
