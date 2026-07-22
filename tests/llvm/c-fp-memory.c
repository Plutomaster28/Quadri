double seabird_fp_roundtrip(volatile double *address, double value) {
    *address = value;
    return *address;
}
