#include "seabird_console.h"

int main(void) {
    sb_print_string("N: ");
    long limit = sb_read_int();
    long value = 1;
    while (value <= limit) {
        sb_print_int(value);
        sb_print_char(value == limit ? '\n' : ' ');
        ++value;
    }
    return 0;
}
