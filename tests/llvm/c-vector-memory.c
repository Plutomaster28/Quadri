typedef unsigned long long u64;
typedef u64 u64x2 __attribute__((vector_size(16)));

u64x2 seabird_vector_roundtrip(volatile u64x2 *address, u64x2 value) {
    *address = value;
    return *address;
}
