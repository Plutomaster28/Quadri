define i64 @large_switch(i64 %value) {
entry:
  switch i64 %value, label %default [
    i64 0, label %case0
    i64 1, label %case1
    i64 2, label %case2
    i64 3, label %case3
    i64 4, label %case4
    i64 5, label %case5
    i64 6, label %case6
    i64 7, label %case7
    i64 8, label %case8
    i64 9, label %case9
    i64 10, label %case10
    i64 11, label %case11
    i64 12, label %case12
    i64 13, label %case13
    i64 14, label %case14
    i64 15, label %case15
  ]
case0: ret i64 3
case1: ret i64 5
case2: ret i64 7
case3: ret i64 11
case4: ret i64 13
case5: ret i64 17
case6: ret i64 19
case7: ret i64 23
case8: ret i64 29
case9: ret i64 31
case10: ret i64 37
case11: ret i64 41
case12: ret i64 43
case13: ret i64 47
case14: ret i64 53
case15: ret i64 59
default: ret i64 -1
}
