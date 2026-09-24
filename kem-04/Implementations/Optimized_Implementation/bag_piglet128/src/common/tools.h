#ifndef TOOLS_H
#define TOOLS_H

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(__linux__)
#include <unistd.h>
#elif defined(_WIN32)
#include <intrin.h>
#include <windows.h>
#endif

static inline uint64_t cputimer(void) {
#if defined(_WIN32)
  return __rdtsc();
#else
  unsigned int lo;
  unsigned int hi;
  __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t) hi << 32) | lo;
#endif
}

extern uint64_t TM_start;
extern uint64_t TM_end;
extern uint64_t *TM_mem;
extern uint64_t *TM_mem1;
extern uint64_t *TM_mem2;
extern uint64_t *TM_mem3;

int tools_prepare_timing(size_t loop_count, int series_count);
void Analyis_TM(int loop, const char *label);
void Analyis_TM_3(int loop, const char *label1, const char *label2,
                  const char *label3);
void print_bytes(const uint8_t *arr, int32_t len);
void PrintLog(const char *message, const char *file, int line,
              const char *function);

#define Timer(label, code)                                                \
  do {                                                                    \
    TM_start = cputimer();                                                 \
    code;                                                                 \
    TM_end = cputimer();                                                   \
    printf("Running Time of <%s>:\t%" PRIu64 " cycles\n",              \
           (label), TM_end - TM_start);                                    \
  } while(0)

#define Loop(loop_count, label, code)                                     \
  do {                                                                    \
    int _loop_count = (loop_count);                                        \
    if(tools_prepare_timing((size_t) _loop_count, 1) != 0) {               \
      abort();                                                             \
    }                                                                      \
    for(int _i = 0; _i < _loop_count; ++_i) {                             \
      TM_start = cputimer();                                               \
      code;                                                               \
      TM_end = cputimer();                                                 \
      TM_mem[_i] = TM_end - TM_start;                                      \
    }                                                                      \
    (void) (label);                                                        \
  } while(0)

#define LOOP(loop_count, label1, code1, label2, code2, label3, code3)     \
  do {                                                                    \
    int _loop_count = (loop_count);                                        \
    if(tools_prepare_timing((size_t) _loop_count, 3) != 0) {               \
      abort();                                                             \
    }                                                                      \
    for(int _i = 0; _i < _loop_count; ++_i) {                             \
      TM_start = cputimer();                                               \
      code1;                                                              \
      TM_end = cputimer();                                                 \
      TM_mem1[_i] = TM_end - TM_start;                                     \
      TM_start = cputimer();                                               \
      code2;                                                              \
      TM_end = cputimer();                                                 \
      TM_mem2[_i] = TM_end - TM_start;                                     \
      TM_start = cputimer();                                               \
      code3;                                                              \
      TM_end = cputimer();                                                 \
      TM_mem3[_i] = TM_end - TM_start;                                     \
    }                                                                      \
    Analyis_TM_3(_loop_count, (label1), (label2), (label3));               \
  } while(0)

#define pn puts("")
#define where printf("File: %s, Line: %d, Function: %s. ",                \
                     __FILE__, __LINE__, __func__)
#define ErrorInfo(format, ...)                                             \
  do {                                                                     \
    printf("[Error] ");                                                    \
    where;                                                                 \
    printf((format), ##__VA_ARGS__);                                       \
    pn;                                                                    \
  } while(0)

#define LOGFILE "Log.txt"
#define Log(message) PrintLog((message), __FILE__, __LINE__, __func__)

#endif
