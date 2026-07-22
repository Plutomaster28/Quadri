__attribute__((noinline)) int seabird_f32_lt(float lhs, float rhs) {
  return lhs < rhs;
}
__attribute__((noinline)) int seabird_f32_eq(float lhs, float rhs) {
  return lhs == rhs;
}
__attribute__((noinline)) int seabird_f32_unordered(float lhs, float rhs) {
  return __builtin_isunordered(lhs, rhs);
}
__attribute__((noinline)) float
seabird_f32_select(float lhs, float rhs, float yes, float no) {
  return lhs <= rhs ? yes : no;
}

__attribute__((noinline)) int seabird_f64_gt(double lhs, double rhs) {
  return lhs > rhs;
}
__attribute__((noinline)) int seabird_f64_ne(double lhs, double rhs) {
  return lhs != rhs;
}
__attribute__((noinline)) double
seabird_f64_select(double lhs, double rhs, double yes, double no) {
  return lhs >= rhs ? yes : no;
}

long seabird_fp_compare_call(void) {
  volatile float one_f = 1.0f;
  volatile float two_f = 2.0f;
  volatile float nan_f = __builtin_nanf("");
  volatile double two_d = 2.0;
  volatile double three_d = 3.0;
  volatile double nan_d = __builtin_nan("");
  return seabird_f32_lt(one_f, two_f) +
         seabird_f32_eq(two_f, two_f) +
         seabird_f32_unordered(nan_f, one_f) +
         (long)seabird_f32_select(one_f, two_f, 7.0f, 9.0f) +
         seabird_f64_gt(three_d, two_d) + seabird_f64_ne(nan_d, two_d) +
         (long)seabird_f64_select(three_d, two_d, 11.0, 13.0);
}
