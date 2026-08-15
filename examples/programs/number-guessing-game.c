#include "seabird_console.h"

int main(void) {
    const long secret = 42;
    for (;;) {
        sb_print_string("Guess: ");
        long guess = sb_read_int();
        if (guess < secret)
            sb_print_line("Too low");
        else if (guess > secret)
            sb_print_line("Too high");
        else {
            sb_print_line("Correct");
            return 0;
        }
    }
}
