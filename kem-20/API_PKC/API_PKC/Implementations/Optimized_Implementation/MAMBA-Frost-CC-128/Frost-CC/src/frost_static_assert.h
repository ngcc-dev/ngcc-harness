#ifndef FROST_STATIC_ASSERT_H
#define FROST_STATIC_ASSERT_H

#define FROST_SA_CAT_(a, b) a##b
#define FROST_SA_CAT(a, b) FROST_SA_CAT_(a, b)

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define FROST_STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)
#else
#define FROST_STATIC_ASSERT(cond, msg) \
    typedef char FROST_SA_CAT(frost_static_assertion_, __LINE__)[(cond) ? 1 : -1]
#endif

#endif
