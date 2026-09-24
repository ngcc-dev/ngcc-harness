#if RADIX == 32
#if defined(SQISIGN_GF_IMPL_BROADWELL)
#define NWORDS_FIELD 32
#else
#define NWORDS_FIELD 36
#endif
#define NWORDS_ORDER 32
#elif RADIX == 64
#if defined(SQISIGN_GF_IMPL_BROADWELL)
#define NWORDS_FIELD 16
#else
#define NWORDS_FIELD 17
#endif
#define NWORDS_ORDER 16
#endif
#define BITS 1024
#define LOG2P 10
