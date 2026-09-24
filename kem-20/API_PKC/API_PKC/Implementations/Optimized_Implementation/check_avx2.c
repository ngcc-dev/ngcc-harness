/* Runtime CPU feature check for the optimized AVX2 Frost API_PKC build. */
#include <stdio.h>
#include <stdlib.h>

#if defined(__GNUC__) || defined(__clang__)
int main(void)
{
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
    __builtin_cpu_init();
    if (!__builtin_cpu_supports("avx2") || !__builtin_cpu_supports("aes")) {
        fprintf(stderr, "ERROR: this optimized Frost build requires AVX2 and AES-NI CPU support.\n");
        return EXIT_FAILURE;
    }
    printf("CPU feature check: AVX2 and AES-NI available.\n");
    return EXIT_SUCCESS;
#else
    fprintf(stderr, "ERROR: this optimized Frost build is intended for x86/x86_64 AVX2 targets.\n");
    return EXIT_FAILURE;
#endif
}
#else
int main(void)
{
    fprintf(stderr, "ERROR: CPU feature check requires GCC or Clang builtins.\n");
    return EXIT_FAILURE;
}
#endif
