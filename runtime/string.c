typedef unsigned long long size_t;

__attribute__((optnone, noinline))
void *memset(void *destination, int value, size_t count) {
    unsigned char *out = (unsigned char *)destination;
    for (size_t i = 0; i < count; ++i)
        out[i] = (unsigned char)value;
    return destination;
}

__attribute__((optnone, noinline))
void *memcpy(void *destination, const void *source, size_t count) {
    unsigned char *out = (unsigned char *)destination;
    const unsigned char *in = (const unsigned char *)source;
    for (size_t i = 0; i < count; ++i)
        out[i] = in[i];
    return destination;
}

__attribute__((optnone, noinline))
size_t strlen(const char *text) {
    const unsigned char *bytes = (const unsigned char *)text;
    size_t length = 0;
    while (bytes[length] != 0)
        ++length;
    return length;
}
