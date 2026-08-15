#include "seabird_console.h"

int main(void) {
    sb_print_string("First number: ");
    long lhs = sb_read_int();
    sb_print_string("Operator (+ - * /): ");
    int operation = sb_read_char();
    sb_print_string("Second number: ");
    long rhs = sb_read_int();
    long result = 0;

    if (operation == '+')
        result = lhs + rhs;
    else if (operation == '-')
        result = lhs - rhs;
    else if (operation == '*')
        result = lhs * rhs;
    else if (operation == '/') {
        if (rhs == 0) {
            sb_print_line("Division by zero");
            return 1;
        }
        result = lhs / rhs;
    } else {
        sb_print_line("Unknown operator");
        return 1;
    }

    sb_print_string("Result: ");
    sb_print_int(result);
    sb_print_char('\n');
    return 0;
}
