declare void @llvm.va_start(ptr)
declare void @llvm.va_end(ptr)

define i64 @sum_three(i64 %base, ...) {
entry:
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  %a = va_arg ptr %ap, i64
  %b = va_arg ptr %ap, i64
  %c = va_arg ptr %ap, i64
  call void @llvm.va_end(ptr %ap)
  %ab = add i64 %base, %a
  %abc = add i64 %ab, %b
  %result = add i64 %abc, %c
  ret i64 %result
}

define i64 @call_sum_three() {
entry:
  %result = call i64 (i64, ...) @sum_three(i64 1, i64 10, i64 20, i64 30)
  ret i64 %result
}

define i32 @sum_two_ints(i32 %base, ...) {
entry:
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  %a = va_arg ptr %ap, i32
  %b = va_arg ptr %ap, i32
  call void @llvm.va_end(ptr %ap)
  %base_a = add i32 %base, %a
  %result = add i32 %base_a, %b
  ret i32 %result
}

define i32 @call_sum_two_ints() {
entry:
  %result = call i32 (i32, ...) @sum_two_ints(i32 1, i32 10, i32 20)
  ret i32 %result
}

define double @sum_two_fp(double %base, ...) {
entry:
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  %a = va_arg ptr %ap, double
  %b = va_arg ptr %ap, double
  call void @llvm.va_end(ptr %ap)
  %base_a = fadd double %base, %a
  %result = fadd double %base_a, %b
  ret double %result
}

define double @call_sum_two_fp() {
entry:
  %result = call double (double, ...) @sum_two_fp(
      double 1.500000e+00, double 2.250000e+00, double 3.250000e+00)
  ret double %result
}
