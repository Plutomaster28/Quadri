typedef unsigned long long u64;
typedef long long i64;

static __attribute__((noinline)) u64 yes(u64 value) { return value + 1; }
static __attribute__((noinline)) u64 no(u64 value) { return value + 2; }

u64 seabird_slt(i64 a, i64 b) { return a < b ? yes((u64)a) : no((u64)b); }
u64 seabird_sge(i64 a, i64 b) { return a >= b ? yes((u64)a) : no((u64)b); }
u64 seabird_ugt(u64 a, u64 b) { return a > b ? yes(a) : no(b); }
u64 seabird_ule(u64 a, u64 b) { return a <= b ? yes(a) : no(b); }
