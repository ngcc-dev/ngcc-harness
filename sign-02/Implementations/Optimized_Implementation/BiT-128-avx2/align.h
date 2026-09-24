/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#ifndef ALIGN_H
#define ALIGN_H

#if defined(_MSC_VER)
    #define ALIGNED_ATTR(x) __declspec(align(x))
#elif defined(__GNUC__) || defined(__clang__)
    #define ALIGNED_ATTR(x) __attribute__((aligned(x)))
#else
    #define ALIGNED_ATTR(x)
#endif

#define ALIGNED_32 ALIGNED_ATTR(32)

#if defined(_MSC_VER)
    #define RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
    #define RESTRICT __restrict__
#else
    #define RESTRICT
#endif

#if defined(_MSC_VER)
    #define RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
    #define RESTRICT __restrict__
#else
    #define RESTRICT
#endif

#if defined(__clang__)
    #define PRAGMA_UNROLL_2  _Pragma("clang loop unroll_count(2)")
    #define PRAGMA_UNROLL_4  _Pragma("clang loop unroll_count(4)")
    #define PRAGMA_UNROLL_8  _Pragma("clang loop unroll_count(8)")
#elif defined(__GNUC__)
    #define PRAGMA_UNROLL_2  _Pragma("GCC unroll 2")
    #define PRAGMA_UNROLL_4  _Pragma("GCC unroll 4")
    #define PRAGMA_UNROLL_8  _Pragma("GCC unroll 8")
#elif defined(_MSC_VER) && !defined(__clang__)
    #define PRAGMA_UNROLL_2  _Pragma("loop(no_vector)")
    #define PRAGMA_UNROLL_4  _Pragma("loop(no_vector)")
    #define PRAGMA_UNROLL_8  _Pragma("loop(no_vector)")
#else
    #define PRAGMA_UNROLL_2
    #define PRAGMA_UNROLL_4
    #define PRAGMA_UNROLL_8
#endif


#endif // ALIGN_H