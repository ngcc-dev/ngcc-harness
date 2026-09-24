#ifndef LOG_H
#define LOG_H

#if DEBUG

#include <stdio.h>

#define LOG(x, ...) do { fprintf(stderr, "(%s) " x "\n\n", __func__, ##__VA_ARGS__); } while (0)

#define LOG_VAL(x) do { fprintf(stderr, "(%s) " #x ":\n%i\n\n", __func__, x); } while (0)

#define LOG_VEC(v, len, ...) do { \
  if (sizeof( (char[]){#__VA_ARGS__} ) == 1) \
    fprintf(stderr, "(%s) " #v ":\n[", __func__); \
  else \
    fprintf(stderr, "(%s) " __VA_ARGS__ ":\n[", __func__); \
  fprintf(stderr, "%i", v[0]); \
  for (int _log_i = 1; _log_i < len; _log_i++) \
    fprintf(stderr, ", %i", v[_log_i]); \
  fprintf(stderr, "]\n\n"); \
} while (0)

#define LOG_VEC_FMT(v, len, fmt, ...) do { \
  fprintf(stderr, "(%s) " fmt ":\n[", __func__, ##__VA_ARGS__); \
  fprintf(stderr, "%i", (v)[0]); \
  for (int _log_i = 1; _log_i < (len); _log_i++) \
    fprintf(stderr, ", %i", (v)[_log_i]); \
  fprintf(stderr, "]\n\n"); \
} while (0)

#define LOG_HEX(v, len, ...) do { \
  if (sizeof( (char[]){#__VA_ARGS__} ) == 1) \
    fprintf(stderr, "(%s) " #v ":\n0x", __func__); \
  else \
    fprintf(stderr, "(%s) " __VA_ARGS__ ":\n0x", __func__); \
  for (int _log_i = 0; _log_i < len; _log_i++) \
    fprintf(stderr, "%02x", v[_log_i]); \
  fprintf(stderr, "\n\n"); \
} while (0)

#define LOG_MAT(v, m, n, ...) do { \
  if (sizeof( (char[]){#__VA_ARGS__} ) == 1) \
    fprintf(stderr, "(%s) " #v ":\n", __func__); \
  else \
    fprintf(stderr, "(%s) " __VA_ARGS__ ":\n", __func__); \
  pmod_mat_fprint(stderr, v, m, n); \
  fprintf(stderr, "\n"); \
} while (0)

#define LOG_MAT_FMT(v, m, n, f, ...) do { \
  fprintf(stderr, "(%s) " f ":\n", __func__, ##__VA_ARGS__); \
  pmod_mat_fprint(stderr, v, m, n); \
  fprintf(stderr, "\n"); \
} while (0)


#else

#define LOG(x, ...) do { } while(0);

#define LOG_VAL(v, ...) do { } while(0);

#define LOG_VEC(v, len, ...) do { } while(0);
#define LOG_VEC_FMT(v, len, fmt, ...) do { } while(0);

#define LOG_HEX(v, len, ...) do { } while(0);

#define LOG_MAT(v, m, n, ...) do { } while(0);
#define LOG_MAT_FMT(v, m, n, fmt, ...) do { } while(0);

#endif

#endif

