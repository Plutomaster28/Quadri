#include "seabird_console.h"

int main(void) {
    const long password = 7319;
    sb_print_string("Password: ");
    long entered = sb_read_int();
    if (entered == password)
        sb_print_line("Access Granted");
    else
        sb_print_line("Access Denied");
    return 0;
}
