typedef unsigned long long u64;

extern u64 seabird_link_add(u64, u64);

u64 seabird_link_entry(u64 value) {
    return seabird_link_add(value, 5) ^ 3;
}
