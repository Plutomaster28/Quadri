define i64 @gp_signed_less(i64 %lhs, i64 %rhs) {
entry:
  %condition = icmp slt i64 %lhs, %rhs
  %result = zext i1 %condition to i64
  ret i64 %result
}

define i64 @gp_unsigned_less(i64 %lhs, i64 %rhs) {
entry:
  %condition = icmp ult i64 %lhs, %rhs
  %result = zext i1 %condition to i64
  ret i64 %result
}

define i64 @gp_equal(i64 %lhs, i64 %rhs) {
entry:
  %condition = icmp eq i64 %lhs, %rhs
  %result = zext i1 %condition to i64
  ret i64 %result
}

define i64 @gp_select(i1 %condition, i64 %if_true, i64 %if_false) {
entry:
  %result = select i1 %condition, i64 %if_true, i64 %if_false
  ret i64 %result
}

define i64 @gp_compare_select(i64 %lhs, i64 %rhs) {
entry:
  %condition = icmp uge i64 %lhs, %rhs
  %result = select i1 %condition, i64 %lhs, i64 %rhs
  ret i64 %result
}
