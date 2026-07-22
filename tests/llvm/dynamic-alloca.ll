define i64 @dynamic_alloca(i64 %size) {
entry:
  %buffer = alloca i8, i64 %size, align 16
  store volatile i8 42, ptr %buffer, align 1
  %address = ptrtoint ptr %buffer to i64
  ret i64 %address
}

declare void @dynamic_barrier(ptr)
declare void @dynamic_barrier2(ptr, ptr)

define void @dynamic_alloca_call(i64 %size) {
entry:
  %buffer = alloca i8, i64 %size, align 16
  call void @dynamic_barrier(ptr %buffer)
  ret void
}

define i64 @dynamic_alloca_with_fixed(i64 %size, i64 %value) {
entry:
  %fixed = alloca i64, align 8
  %dynamic = alloca i8, i64 %size, align 16
  store volatile i64 %value, ptr %fixed, align 8
  call void @dynamic_barrier2(ptr %fixed, ptr %dynamic)
  %result = load volatile i64, ptr %fixed, align 8
  ret i64 %result
}
