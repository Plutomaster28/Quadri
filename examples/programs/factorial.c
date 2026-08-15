#include "seabird_console.h"

int main(void) {
    sb_print_string("N (0-10): ");
    long number = sb_read_int();
    if (number < 0 || number > 10) {
        sb_print_line("Out of range");
        return 1;
    }

    long result = 1;
    for (long value = 2; value <= number; ++value)
        result *= value;
    sb_print_string("Factorial: ");
    sb_print_int(result);
    sb_print_char('\n');
    return 0;
}
