typedef unsigned long long u64;

static __attribute__((noinline)) u64 plus_one(u64 value) {
    return value + 1;
}

static __attribute__((noinline)) u64 plus_two(u64 value) {
    return value + 2;
}

u64 seabird_choose(u64 a, u64 b) {
    if (a == b)
        return plus_one(a);
    return plus_two(b);
}

u64 seabird_different(u64 a, u64 b) {
    if (a != b)
        return plus_one(a ^ b);
    return plus_two(0);
}
