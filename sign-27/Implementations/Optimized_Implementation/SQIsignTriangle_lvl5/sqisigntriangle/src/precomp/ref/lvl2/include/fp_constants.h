#if RADIX == 32
#if defined(SQISIGN_GF_IMPL_BROADWELL)
#define NWORDS_FIELD 10
#else
#define NWORDS_FIELD 11
#endif
#define NWORDS_ORDER 10
#elif RADIX == 64
#if defined(SQISIGN_GF_IMPL_BROADWELL)
#define NWORDS_FIELD 5
#else
#define NWORDS_FIELD 6
#endif
#define NWORDS_ORDER 5
#endif
#define BITS 320
#define LOG2P 9
