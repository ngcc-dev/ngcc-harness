#ifndef SM4TH_SIG_TIMING_H
#define SM4TH_SIG_TIMING_H

#ifndef SM4TH_COMPONENT_TIMING
#define SM4TH_COMPONENT_TIMING 0
#endif

#if SM4TH_COMPONENT_TIMING
double sm4th_timing_now_ms(void);
void sm4th_timing_reset(const char* op);
void sm4th_timing_record(const char* label, double elapsed_ms);
void sm4th_timing_report(const char* op);

#define SIG_TSTART(name) const double name = sm4th_timing_now_ms()
#define SIG_TEND(label, name) sm4th_timing_record((label), sm4th_timing_now_ms() - (name))
#else
static inline void sm4th_timing_reset(const char* op) {
  (void)op;
}
static inline void sm4th_timing_report(const char* op) {
  (void)op;
}

#define SIG_TSTART(name)
#define SIG_TEND(label, name)
#endif

#endif
