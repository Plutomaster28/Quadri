#include "seabird_console.h"

int main(void) {
    volatile long values[] = {7, 3, 19, 4, 12, 8};
    const long count = (long)(sizeof(values) / sizeof(values[0]));
    long sum = 0;
    long maximum = values[0];

    for (long index = 0; index < count; ++index) {
        long value = values[index];
        sum += value;
        if (value > maximum)
            maximum = value;
    }

    sb_print_string("Sum: ");
    sb_print_int(sum);
    sb_print_string(" Max: ");
    sb_print_int(maximum);
    sb_print_char('\n');
    return 0;
}
