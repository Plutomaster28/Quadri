target triple = "seabird64-unknown-none"

declare double @llvm.fma.f64(double, double, double)
declare double @llvm.minnum.f64(double, double)
declare double @llvm.maxnum.f64(double, double)

define double @seabird_fmadd(double %a, double %b, double %c) {
entry:
  %result = call double @llvm.fma.f64(double %a, double %b, double %c)
  ret double %result
}

define double @seabird_fmsub(double %a, double %b, double %c) {
entry:
  %negative = fneg double %c
  %result = call double @llvm.fma.f64(double %a, double %b, double %negative)
  ret double %result
}

define double @seabird_fmin(double %a, double %b) {
entry:
  %result = call double @llvm.minnum.f64(double %a, double %b)
  ret double %result
}

define double @seabird_fmax(double %a, double %b) {
entry:
  %result = call double @llvm.maxnum.f64(double %a, double %b)
  ret double %result
}
