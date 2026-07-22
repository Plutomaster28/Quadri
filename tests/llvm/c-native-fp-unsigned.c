__attribute__((noinline)) float seabird_u64_to_f32(unsigned long value) {
  return (float)value;
}

__attribute__((noinline)) double seabird_u64_to_f64(unsigned long value) {
  return (double)value;
}

__attribute__((noinline)) unsigned long seabird_f32_to_u64(float value) {
  return (unsigned long)value;
}

__attribute__((noinline)) unsigned long seabird_f64_to_u64(double value) {
  return (unsigned long)value;
}

long seabird_fp_unsigned_call(void) {
  volatile unsigned long maximum = ~0UL;
  volatile unsigned long high = 1UL << 63;
  float maximum_f = seabird_u64_to_f32(maximum);
  double high_d = seabird_u64_to_f64(high);
  unsigned long from_f = seabird_f32_to_u64(0x1p63f);
  unsigned long from_d = seabird_f64_to_u64(0x1p63);
  return (maximum_f == 0x1p64f) + (high_d == 0x1p63) +
         (from_f == high) + (from_d == high);
}
