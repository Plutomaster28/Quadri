target triple = "seabird64-unknown-none"

declare <2 x i64> @llvm.abs.v2i64(<2 x i64>, i1)
declare <2 x i64> @llvm.smax.v2i64(<2 x i64>, <2 x i64>)
declare <2 x i64> @llvm.smin.v2i64(<2 x i64>, <2 x i64>)

define <2 x i64> @seabird_vdiv(<2 x i64> %lhs, <2 x i64> %rhs) {
entry:
  %result = udiv <2 x i64> %lhs, %rhs
  ret <2 x i64> %result
}

define <2 x i64> @seabird_vabs(<2 x i64> %value) {
entry:
  %result = call <2 x i64> @llvm.abs.v2i64(<2 x i64> %value, i1 false)
  ret <2 x i64> %result
}

define <2 x i64> @seabird_vmax(<2 x i64> %lhs, <2 x i64> %rhs) {
entry:
  %result = call <2 x i64> @llvm.smax.v2i64(<2 x i64> %lhs, <2 x i64> %rhs)
  ret <2 x i64> %result
}

define <2 x i64> @seabird_vmin(<2 x i64> %lhs, <2 x i64> %rhs) {
entry:
  %result = call <2 x i64> @llvm.smin.v2i64(<2 x i64> %lhs, <2 x i64> %rhs)
  ret <2 x i64> %result
}
