define i32 @window_sum8(i32 %a, i32 %b, i32 %c, i32 %d,
                        i32 %e, i32 %f, i32 %g, i32 %h) {
  %ab = add i32 %a, %b
  %cd = add i32 %c, %d
  %ef = add i32 %e, %f
  %gh = add i32 %g, %h
  %abcd = add i32 %ab, %cd
  %efgh = add i32 %ef, %gh
  %result = add i32 %abcd, %efgh
  ret i32 %result
}

define i32 @window_call8(i32 %x) {
  %result = call i32 @window_sum8(i32 %x, i32 2, i32 3, i32 4,
                                  i32 5, i32 6, i32 7, i32 8)
  ret i32 %result
}
