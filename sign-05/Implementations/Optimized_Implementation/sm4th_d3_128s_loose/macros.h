/*
 * basic macros: MIN, MAX, and ARRAY_SIZE
 */

#ifndef MACROS_H
#define MACROS_H

#if !defined(MIN)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#if !defined(MAX)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* number of elements in an array */
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#endif
