#include "seabird_console.h"

int main(void) {
    const long computer = 2; /* paper */
    sb_print_string("Choose 1=rock 2=paper 3=scissors: ");
    long player = sb_read_int();
    if (player < 1 || player > 3) {
        sb_print_line("Invalid choice");
        return 1;
    }

    sb_print_string("Computer: 2 Result: ");
    if (player == computer)
        sb_print_line("Draw");
    else if (player == 3)
        sb_print_line("You win");
    else
        sb_print_line("Computer wins");
    return 0;
}
