declare void @hot_path()
declare void @cold_path()

define void @weighted_loop(i64 %count) {
entry:
  br label %loop
loop:
  %remaining = phi i64 [ %count, %entry ], [ %next, %loop ]
  call void @hot_path()
  %next = add i64 %remaining, -1
  %continue = icmp ne i64 %next, 0
  br i1 %continue, label %loop, label %exit, !prof !0
exit:
  ret void
}

define void @weighted_likely(i1 %condition) {
entry:
  br i1 %condition, label %hot, label %exit, !prof !0
hot:
  call void @hot_path()
  br label %exit
exit:
  ret void
}

define void @weighted_unlikely(i1 %condition) {
entry:
  br i1 %condition, label %cold, label %exit, !prof !1
cold:
  call void @cold_path()
  br label %exit
exit:
  ret void
}

!0 = !{!"branch_weights", i32 1000, i32 1}
!1 = !{!"branch_weights", i32 1, i32 1000}
