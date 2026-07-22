typedef unsigned long long u64;
typedef u64 u64x2 __attribute__((vector_size(16)));

double seabird_fp_mix(double a, double b, double c) {
    return (a + b) * c;
}

u64x2 seabird_vector_mix(u64x2 a, u64x2 b) {
    return (a + b) ^ a;
}
