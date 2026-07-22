target triple = "seabird32-unknown-none"

define i32 @tritium_mix(i32 %a, i32 %b, i32 %c) {
entry:
  %sum = add i32 %a, %b
  %mixed = xor i32 %sum, %c
  ret i32 %mixed
}

define i32 @tritium_choose(i32 %a, i32 %b) {
entry:
  %is_less = icmp slt i32 %a, %b
  br i1 %is_less, label %less, label %not_less

less:
  %added = add i32 %a, %b
  ret i32 %added

not_less:
  %subtracted = sub i32 %a, %b
  ret i32 %subtracted
}

define i32 @tritium_constant() {
entry:
  ret i32 305419896
}

define i32 @tritium_unsigned_less(i32 %a, i32 %b) {
entry:
  %condition = icmp ult i32 %a, %b
  %result = zext i1 %condition to i32
  ret i32 %result
}

define i32 @tritium_equal(i32 %a, i32 %b) {
entry:
  %condition = icmp eq i32 %a, %b
  %result = zext i1 %condition to i32
  ret i32 %result
}

define i32 @tritium_div32(i32 %a, i32 %b) {
entry:
  %result = sdiv i32 %a, %b
  ret i32 %result
}

define i32 @tritium_mod32(i32 %a, i32 %b) {
entry:
  %result = srem i32 %a, %b
  ret i32 %result
}

define i32 @tritium_udiv32(i32 %a, i32 %b) {
entry:
  %result = udiv i32 %a, %b
  ret i32 %result
}

define i32 @tritium_addi32(i32 %value) {
entry:
  %result = add i32 %value, 123456
  ret i32 %result
}

define i32 @tritium_subi32(i32 %value) {
entry:
  %result = sub i32 %value, 2345
  ret i32 %result
}

define i32 @tritium_muli32(i32 %value) #0 {
entry:
  %result = mul i32 %value, 37
  ret i32 %result
}

define i32 @tritium_divi32(i32 %value) #0 {
entry:
  %result = sdiv i32 %value, 7
  ret i32 %result
}

define i32 @tritium_modi32(i32 %value) #0 {
entry:
  %result = srem i32 %value, 7
  ret i32 %result
}

define i32 @tritium_mask32(i32 %value) {
entry:
  %result = and i32 %value, 65535
  ret i32 %result
}

define i32 @tritium_cmpi32(i32 %value) {
entry:
  %condition = icmp eq i32 %value, 123456
  br i1 %condition, label %equal, label %not_equal

equal:
  ret i32 1

not_equal:
  ret i32 0
}

declare i32 @llvm.abs.i32(i32, i1 immarg)
declare i32 @llvm.ctlz.i32(i32, i1 immarg)
declare i32 @llvm.cttz.i32(i32, i1 immarg)
declare i32 @llvm.ctpop.i32(i32)
declare i32 @llvm.sadd.sat.i32(i32, i32)
declare i32 @llvm.uadd.sat.i32(i32, i32)
declare i32 @llvm.ssub.sat.i32(i32, i32)
declare i32 @llvm.usub.sat.i32(i32, i32)
declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i32 @llvm.fshr.i32(i32, i32, i32)
declare i32 @llvm.smax.i32(i32, i32)
declare i32 @llvm.smin.i32(i32, i32)

define i32 @tritium_neg32(i32 %value) {
entry:
  %result = sub i32 0, %value
  ret i32 %result
}

define i32 @tritium_inc32(i32 %value) {
entry:
  %result = add i32 %value, 1
  ret i32 %result
}

define i32 @tritium_dec32(i32 %value) {
entry:
  %result = add i32 %value, -1
  ret i32 %result
}

define i32 @tritium_not32(i32 %value) {
entry:
  %result = xor i32 %value, -1
  ret i32 %result
}

define i32 @tritium_abs32(i32 %value) {
entry:
  %result = call i32 @llvm.abs.i32(i32 %value, i1 false)
  ret i32 %result
}

define i32 @tritium_clz32(i32 %value) {
entry:
  %result = call i32 @llvm.ctlz.i32(i32 %value, i1 false)
  ret i32 %result
}

define i32 @tritium_ctz32(i32 %value) {
entry:
  %result = call i32 @llvm.cttz.i32(i32 %value, i1 false)
  ret i32 %result
}

define i32 @tritium_popc32(i32 %value) {
entry:
  %result = call i32 @llvm.ctpop.i32(i32 %value)
  ret i32 %result
}

define i32 @tritium_adds32(i32 %a, i32 %b) {
entry:
  %result = call i32 @llvm.sadd.sat.i32(i32 %a, i32 %b)
  ret i32 %result
}

define i32 @tritium_addu32(i32 %a, i32 %b) {
entry:
  %result = call i32 @llvm.uadd.sat.i32(i32 %a, i32 %b)
  ret i32 %result
}

define i32 @tritium_subs32(i32 %a, i32 %b) {
entry:
  %result = call i32 @llvm.ssub.sat.i32(i32 %a, i32 %b)
  ret i32 %result
}

define i32 @tritium_subu32(i32 %a, i32 %b) {
entry:
  %result = call i32 @llvm.usub.sat.i32(i32 %a, i32 %b)
  ret i32 %result
}

define i32 @tritium_rol32(i32 %value, i32 %amount) {
entry:
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 %amount)
  ret i32 %result
}

define i32 @tritium_ror32(i32 %value, i32 %amount) {
entry:
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 %amount)
  ret i32 %result
}

define i32 @tritium_max32(i32 %a, i32 %b) {
entry:
  %result = call i32 @llvm.smax.i32(i32 %a, i32 %b)
  ret i32 %result
}

define i32 @tritium_min32(i32 %a, i32 %b) {
entry:
  %result = call i32 @llvm.smin.i32(i32 %a, i32 %b)
  ret i32 %result
}

attributes #0 = { noinline optnone }

define i64 @tritium_add64(i64 %a, i64 %b) {
entry:
  %sum = add i64 %a, %b
  ret i64 %sum
}

define i64 @tritium_sub64(i64 %a, i64 %b) {
entry:
  %difference = sub i64 %a, %b
  ret i64 %difference
}

define i64 @tritium_shl64(i64 %value, i64 %amount) {
entry:
  %result = shl i64 %value, %amount
  ret i64 %result
}

define i64 @tritium_lshr64(i64 %value, i64 %amount) {
entry:
  %result = lshr i64 %value, %amount
  ret i64 %result
}

define i64 @tritium_ashr64(i64 %value, i64 %amount) {
entry:
  %result = ashr i64 %value, %amount
  ret i64 %result
}

define i64 @tritium_mul64(i64 %a, i64 %b) {
entry:
  %product = mul i64 %a, %b
  ret i64 %product
}

define i64 @tritium_select64_uge(i64 %a, i64 %b, i64 %yes, i64 %no) {
entry:
  %condition = icmp uge i64 %a, %b
  %result = select i1 %condition, i64 %yes, i64 %no
  ret i64 %result
}

define i64 @tritium_branch64_uge(i64 %a, i64 %b) {
entry:
  %condition = icmp uge i64 %a, %b
  br i1 %condition, label %greater_or_equal, label %less

greater_or_equal:
  %difference = sub i64 %a, %b
  ret i64 %difference

less:
  ret i64 %a
}

define i64 @tritium_udiv64(i64 %a, i64 %b) {
entry:
  %quotient = udiv i64 %a, %b
  ret i64 %quotient
}

define i64 @tritium_sdiv64(i64 %a, i64 %b) {
entry:
  %quotient = sdiv i64 %a, %b
  ret i64 %quotient
}

define i64 @tritium_urem64(i64 %a, i64 %b) {
entry:
  %remainder = urem i64 %a, %b
  ret i64 %remainder
}

define i64 @tritium_srem64(i64 %a, i64 %b) {
entry:
  %remainder = srem i64 %a, %b
  ret i64 %remainder
}

declare i64 @tritium_external64(i64, i64)

define i64 @tritium_call64(i64 %a, i64 %b) {
entry:
  %result = call i64 @tritium_external64(i64 %a, i64 %b)
  ret i64 %result
}

define i32 @tritium_load(ptr %address) {
entry:
  %value = load i32, ptr %address, align 4
  ret i32 %value
}

define i64 @tritium_load64(ptr %address) {
entry:
  %value = load i64, ptr %address, align 4
  ret i64 %value
}

define void @tritium_store(ptr %address, i32 %value) {
entry:
  store i32 %value, ptr %address, align 4
  ret void
}

define void @tritium_store64(ptr %address, i64 %value) {
entry:
  store i64 %value, ptr %address, align 4
  ret void
}

declare i32 @tritium_external(i32, i32, i32, i32, i32, i32, i32)

define i32 @tritium_call_seven(i32 %seed) {
entry:
  %result = call i32 @tritium_external(
      i32 %seed, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6)
  ret i32 %result
}

define i32 @tritium_sum_seven(
    i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6) {
entry:
  %s01 = add i32 %a0, %a1
  %s012 = add i32 %s01, %a2
  %s0123 = add i32 %s012, %a3
  %s01234 = add i32 %s0123, %a4
  %s012345 = add i32 %s01234, %a5
  %sum = add i32 %s012345, %a6
  ret i32 %sum
}

declare i32 @tritium_external_eleven(
    i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)

define i32 @tritium_call_eleven(i32 %seed) {
entry:
  %result = call i32 @tritium_external_eleven(
      i32 %seed, i32 1, i32 2, i32 3, i32 4, i32 5,
      i32 6, i32 7, i32 8, i32 9, i32 10)
  ret i32 %result
}

define i32 @tritium_sum_eleven(
    i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5,
    i32 %a6, i32 %a7, i32 %a8, i32 %a9, i32 %a10) {
entry:
  %s01 = add i32 %a0, %a1
  %s012 = add i32 %s01, %a2
  %s0123 = add i32 %s012, %a3
  %s01234 = add i32 %s0123, %a4
  %s012345 = add i32 %s01234, %a5
  %s0123456 = add i32 %s012345, %a6
  %s01234567 = add i32 %s0123456, %a7
  %s012345678 = add i32 %s01234567, %a8
  %s0123456789 = add i32 %s012345678, %a9
  %sum = add i32 %s0123456789, %a10
  ret i32 %sum
}
