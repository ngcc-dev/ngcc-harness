// test_main.cpp — minimal compile/link test for the GALAS optimized impl.
#include "hash.hpp"
#include <cstdio>

int main() {
    faest::hash_state h;
    h.init(faest::secpar::s256);
    h.update("test", 4);
    uint8_t digest[32];
    h.finalize(digest, 32);
    printf("hash OK: %02x%02x%02x%02x\n", digest[0], digest[1], digest[2], digest[3]);
    printf("OPT COMPILE TEST PASSED\n");
    return 0;
}
