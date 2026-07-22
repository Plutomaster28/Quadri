typedef unsigned long long u64;

extern int main(void);

u64 _start(void) {
    return (unsigned int)main();
}
