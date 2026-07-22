_Static_assert(sizeof(long double) == 16,
               "SeaBird long double must be IEEE binary128");

__attribute__((noinline)) long double
sb_quad_math(long double lhs, long double rhs) {
  return (lhs + rhs) * 2.0L;
}

__attribute__((noinline)) long sb_quad_to_long(long double value) {
  return (long)value;
}

__attribute__((noinline)) long double
sb_quad_select(long double lhs, long double rhs, int choose_lhs) {
  return choose_lhs ? lhs : rhs;
}

__attribute__((noinline)) long double sb_quad_ninth(
    long double a0, long double a1, long double a2, long double a3,
    long double a4, long double a5, long double a6, long double a7,
    long double a8) {
  (void)a0;
  (void)a1;
  (void)a2;
  (void)a3;
  (void)a4;
  (void)a5;
  (void)a6;
  (void)a7;
  return a8;
}

int sb_native_fp128_wrapper(void) {
  volatile long double lhs = 1.5L;
  volatile long double rhs = 2.25L;
  long double result = sb_quad_math(lhs, rhs);
  int score = result == 7.5L;
  score += sb_quad_to_long(result) == 7;
  score += sb_quad_select(lhs, rhs, 0) == rhs;
  score += sb_quad_ninth(1.0L, 2.0L, 3.0L, 4.0L, 5.0L, 6.0L, 7.0L,
                         8.0L, 9.0L) == 9.0L;
  return score;
}
