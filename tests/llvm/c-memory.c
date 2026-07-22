typedef unsigned long long u64;

u64 seabird_load(const u64 *address) {
    return *address;
}

void seabird_store(u64 *address, u64 value) {
    *address = value;
}

u64 seabird_roundtrip(u64 *address, u64 value) {
    *address = value;
    return *address;
}
