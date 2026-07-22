#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

_Static_assert(sizeof(void *) == 4, "Tritium requires 32-bit pointers");
_Static_assert(sizeof(long) == 4, "Tritium requires ILP32");
_Static_assert(sizeof(va_list) == sizeof(void *),
               "SeaBird va_list is a single pointer");

int tritium_native_sum(int base, ...) {
  va_list arguments;
  va_start(arguments, base);
  int a = va_arg(arguments, int);
  int b = va_arg(arguments, int);
  int c = va_arg(arguments, int);
  va_end(arguments);
  return base + a + b + c;
}

int tritium_native_call(void) {
  return tritium_native_sum(1, 10, 20, 30);
}
