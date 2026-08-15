#include "seabird_console.h"

int main(void) {
    sb_print_string("First number: ");
    long first = sb_read_int();
    sb_print_string("Second number: ");
    long second = sb_read_int();
    sb_print_string("Result: ");
    sb_print_int(first + second);
    sb_print_char('\n');
    return 0;
}
