typedef unsigned long long u64;

u64 seabird_mix(u64 a, u64 b, u64 c, u64 d) {
    return ((a + b) ^ c) - d;
}

u64 seabird_constant(u64 value) {
    return value ^ 0x1122334455667788ULL;
}
