declare i64 @llvm.abs.i64(i64, i1 immarg)
declare i64 @llvm.ctlz.i64(i64, i1 immarg)
declare i64 @llvm.cttz.i64(i64, i1 immarg)
declare i64 @llvm.ctpop.i64(i64)

define i64 @gp_mul(i64 %lhs, i64 %rhs) {
  %result = mul i64 %lhs, %rhs
  ret i64 %result
}

define i64 @gp_umulh(i64 %lhs, i64 %rhs) {
  %wide_lhs = zext i64 %lhs to i128
  %wide_rhs = zext i64 %rhs to i128
  %wide_product = mul i128 %wide_lhs, %wide_rhs
  %high = lshr i128 %wide_product, 64
  %result = trunc i128 %high to i64
  ret i64 %result
}

define i64 @gp_sdiv(i64 %lhs, i64 %rhs) {
  %result = sdiv i64 %lhs, %rhs
  ret i64 %result
}

define i64 @gp_udiv(i64 %lhs, i64 %rhs) {
  %result = udiv i64 %lhs, %rhs
  ret i64 %result
}

define i64 @gp_srem(i64 %lhs, i64 %rhs) {
  %result = srem i64 %lhs, %rhs
  ret i64 %result
}

define i64 @gp_urem(i64 %lhs, i64 %rhs) {
  %result = urem i64 %lhs, %rhs
  ret i64 %result
}

define i64 @gp_abs(i64 %value) {
  %result = call i64 @llvm.abs.i64(i64 %value, i1 false)
  ret i64 %result
}

define i64 @gp_clz(i64 %value) {
  %result = call i64 @llvm.ctlz.i64(i64 %value, i1 false)
  ret i64 %result
}

define i64 @gp_ctz(i64 %value) {
  %result = call i64 @llvm.cttz.i64(i64 %value, i1 false)
  ret i64 %result
}

define i64 @gp_popcount(i64 %value) {
  %result = call i64 @llvm.ctpop.i64(i64 %value)
  ret i64 %result
}
