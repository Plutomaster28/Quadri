#ifndef SEABIRD_CONSOLE_H
#define SEABIRD_CONSOLE_H

/*
 * SeaBird v0.1 development-console ABI.
 *
 * SYSCALL number is passed in R0. The first argument is passed in R1 and a
 * scalar result is returned in R0. This deliberately tiny interface is for
 * examples, the reference model, early emulators, and board monitors; it is
 * not yet the final hosted operating-system ABI.
 */
long sb_read_int(void);              /* syscall 1 */
void sb_print_int(long value);       /* syscall 2 */
void sb_print_char(int value);       /* syscall 3 */
void sb_print_string(const char *s); /* syscall 4 */
int sb_read_char(void);              /* syscall 5 */

static inline void sb_print_line(const char *text) {
    sb_print_string(text);
    sb_print_char('\n');
}

#endif
