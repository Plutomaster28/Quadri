typedef unsigned long long size_t;
extern void *memset(void *, int, size_t);

int main(void) {
    unsigned char buffer[8];
    memset(buffer, 5, sizeof(buffer));
    return buffer[0] + 37;
}
