typedef unsigned long long u64;

u64 seabird_stack_local(u64 value) {
    volatile u64 local = value + 1;
    return local ^ 3;
}
