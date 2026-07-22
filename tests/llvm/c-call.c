typedef unsigned long long u64;

static __attribute__((noinline)) u64 seabird_add(u64 a, u64 b) {
    return a + b;
}

u64 seabird_caller(u64 value, u64 addend) {
    return seabird_add(value, addend) ^ 3;
}
