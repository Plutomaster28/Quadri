declare void @llvm.va_start(ptr)
declare void @llvm.va_end(ptr)

define i32 @sum_three32(i32 %base, ...) {
entry:
  %ap = alloca ptr, align 4
  call void @llvm.va_start(ptr %ap)
  %a = va_arg ptr %ap, i32
  %b = va_arg ptr %ap, i32
  %c = va_arg ptr %ap, i32
  call void @llvm.va_end(ptr %ap)
  %ab = add i32 %base, %a
  %abc = add i32 %ab, %b
  %result = add i32 %abc, %c
  ret i32 %result
}

define i32 @call_sum_three32() {
entry:
  %result = call i32 (i32, ...) @sum_three32(i32 1, i32 10, i32 20, i32 30)
  ret i32 %result
}
