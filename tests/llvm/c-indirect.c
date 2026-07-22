typedef unsigned long long u64;
typedef u64 (*operation)(u64, u64);

u64 seabird_indirect(operation fn, u64 a, u64 b) {
    return fn(a, b) ^ 3;
}
