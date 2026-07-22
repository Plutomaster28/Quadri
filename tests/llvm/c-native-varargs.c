#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

_Static_assert(sizeof(void *) == sizeof(long), "SB64 requires LP64");
_Static_assert(sizeof(long double) == 16, "SeaBird long double is binary128");
_Static_assert(sizeof(va_list) == sizeof(void *),
               "SeaBird va_list is a single pointer");

long seabird_native_sum(long base, ...) {
  va_list arguments;
  va_start(arguments, base);
  long a = va_arg(arguments, long);
  int b = va_arg(arguments, int);
  long c = va_arg(arguments, long);
  va_end(arguments);
  return base + a + b + c;
}

long seabird_native_call(void) {
  return seabird_native_sum(1, 10L, 20, 30L);
}

double seabird_native_fp_sum(double base, ...) {
  va_list arguments;
  va_start(arguments, base);
  double a = va_arg(arguments, double);
  double b = va_arg(arguments, double);
  va_end(arguments);
  return base + a + b;
}

double seabird_native_fp_call(void) {
  return seabird_native_fp_sum(1.5, 2.25, 3.25);
}
