#include "seabird_console.h"

int main(void) {
    sb_print_string("Number: ");
    long number = sb_read_int();
    if (number % 2 == 0)
        sb_print_line("Even");
    else
        sb_print_line("Odd");
    return 0;
}
