#include <stddef.h>

_Static_assert(sizeof(float) == 4, "SeaBird binary32 must occupy four bytes");

__attribute__((noinline)) float
seabird_f32_arithmetic(float a, float b, float c) {
  return (a + b) * c;
}

float seabird_f32_memory(float *slot, float value) {
  *slot = value;
  return *slot + 1.25f;
}

__attribute__((noinline)) static float seabird_f32_add(float a, float b) {
  return a + b;
}

float seabird_f32_call(void) {
  volatile float a = 1.5f;
  volatile float b = 2.25f;
  return seabird_f32_add(a, b);
}

float seabird_f32_arithmetic_call(void) {
  volatile float a = 1.5f;
  volatile float b = 2.25f;
  volatile float c = 2.0f;
  return seabird_f32_arithmetic(a, b, c);
}

__attribute__((noinline)) float
seabird_f32_sum_ten(float a0, float a1, float a2, float a3, float a4,
                    float a5, float a6, float a7, float a8, float a9) {
  return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9;
}

float seabird_f32_stack_call(void) {
  volatile float ninth = 9.0f;
  volatile float tenth = 10.0f;
  return seabird_f32_sum_ten(1.0f, 2.0f, 3.0f, 4.0f, 5.0f,
                             6.0f, 7.0f, 8.0f, ninth, tenth);
}

float seabird_f32_from_long(long value) {
  return (float)value;
}

long seabird_f32_to_long(float value) {
  return (long)value;
}

double seabird_f32_widen(float value) {
  return (double)value;
}

float seabird_f32_narrow(double value) {
  return (float)value;
}
