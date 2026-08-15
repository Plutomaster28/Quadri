#include "seabird_console.h"

int main(void) {
    sb_print_string("Height in cm: ");
    long height = sb_read_int();
    sb_print_string("Weight in kg: ");
    long weight = sb_read_int();
    if (height <= 0 || weight <= 0) {
        sb_print_line("Invalid measurements");
        return 1;
    }

    long bmi = (weight * 10000) / (height * height);
    sb_print_string("BMI: ");
    sb_print_int(bmi);
    sb_print_string(" Category: ");
    if (bmi < 18)
        sb_print_line("Under");
    else if (bmi < 25)
        sb_print_line("Normal");
    else
        sb_print_line("Over");
    return 0;
}
