typedef unsigned long long u64;

__attribute__((noinline))
u64 seabird_sum11(u64 a0, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5,
                  u64 a6, u64 a7, u64 a8, u64 a9, u64 a10) {
    return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10;
}

u64 seabird_call11(u64 seed) {
    return seabird_sum11(seed, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
}
