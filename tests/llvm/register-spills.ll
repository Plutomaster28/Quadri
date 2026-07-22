declare void @spill_barrier()

define i64 @gpr_spill_pressure(i64 %a, i64 %b, i64 %c, i64 %d,
                               i64 %e, i64 %f, i64 %g, i64 %h) {
entry:
  %v0 = add i64 %a, 11
  %v1 = add i64 %b, 12
  %v2 = add i64 %c, 13
  %v3 = add i64 %d, 14
  %v4 = add i64 %e, 15
  %v5 = add i64 %f, 16
  %v6 = add i64 %g, 17
  %v7 = add i64 %h, 18
  call void @spill_barrier()
  %s0 = add i64 %v0, %v1
  %s1 = add i64 %v2, %v3
  %s2 = add i64 %v4, %v5
  %s3 = add i64 %v6, %v7
  %s4 = add i64 %s0, %s1
  %s5 = add i64 %s2, %s3
  %result = add i64 %s4, %s5
  ret i64 %result
}

declare void @vector_spill_barrier()

define <2 x i64> @vector_spill_pressure(
    <2 x i64> %a, <2 x i64> %b, <2 x i64> %c, <2 x i64> %d,
    <2 x i64> %e, <2 x i64> %f, <2 x i64> %g, <2 x i64> %h) {
entry:
  call void @vector_spill_barrier()
  %s0 = add <2 x i64> %a, %b
  %s1 = add <2 x i64> %c, %d
  %s2 = add <2 x i64> %e, %f
  %s3 = add <2 x i64> %g, %h
  %s4 = add <2 x i64> %s0, %s1
  %s5 = add <2 x i64> %s2, %s3
  %result = add <2 x i64> %s4, %s5
  ret <2 x i64> %result
}
