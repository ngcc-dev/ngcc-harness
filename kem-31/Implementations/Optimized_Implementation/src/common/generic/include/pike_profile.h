#ifndef PIKE_PROFILE_H
#define PIKE_PROFILE_H

#include <stdint.h>
#include <stdio.h>

typedef enum {
    PIKE_PROFILE_PHASE_NONE = 0,
    PIKE_PROFILE_PHASE_KEYGEN,
    PIKE_PROFILE_PHASE_ENCAPS,
    PIKE_PROFILE_PHASE_DECAPS,
    PIKE_PROFILE_PHASE_ENCRYPT,
    PIKE_PROFILE_PHASE_DECRYPT,
    PIKE_PROFILE_PHASE_ENCODE_DECODE,
    PIKE_PROFILE_PHASE_COUNT
} pike_profile_phase_t;

typedef enum {
    PIKE_PROFILE_PHASE_TOTAL = 0,
    PIKE_PROFILE_FP_ADD,
    PIKE_PROFILE_FP_SUB,
    PIKE_PROFILE_FP_HALF,
    PIKE_PROFILE_FP_MUL,
    PIKE_PROFILE_FP_SQR,
    PIKE_PROFILE_FP_INV,
    PIKE_PROFILE_FP_ENCODE,
    PIKE_PROFILE_FP_DECODE,
    PIKE_PROFILE_FP_DECODE_REDUCE,
    PIKE_PROFILE_FP_NEG,
    PIKE_PROFILE_FP_COPY,
    PIKE_PROFILE_FP_SET_ZERO,
    PIKE_PROFILE_FP_SET_ONE,
    PIKE_PROFILE_FP_SET_SMALL,
    PIKE_PROFILE_FP_TOMONT,
    PIKE_PROFILE_FP_FROMMONT,
    PIKE_PROFILE_FP_MONT_SETONE,
    PIKE_PROFILE_FP_IS_EQUAL,
    PIKE_PROFILE_FP_IS_ZERO,
    PIKE_PROFILE_FP_IS_SQUARE,
    PIKE_PROFILE_FP_SQRT,
    PIKE_PROFILE_FP_SELECT,
    PIKE_PROFILE_FP_CSWAP,
    PIKE_PROFILE_FP2_ADD,
    PIKE_PROFILE_FP2_SUB,
    PIKE_PROFILE_FP2_HALF,
    PIKE_PROFILE_FP2_MUL,
    PIKE_PROFILE_FP2_SQR,
    PIKE_PROFILE_FP2_INV,
    PIKE_PROFILE_FP2_ENCODE,
    PIKE_PROFILE_FP2_DECODE,
    PIKE_PROFILE_EC_XDBL,
    PIKE_PROFILE_EC_XADD,
    PIKE_PROFILE_EC_XDBLADD,
    PIKE_PROFILE_EC_XMUL,
    PIKE_PROFILE_EC_MUL,
    PIKE_PROFILE_EC_XDBLMUL,
    PIKE_PROFILE_EC_DBL,
    PIKE_PROFILE_EC_ADD,
    PIKE_PROFILE_EC_LIFT_POINT,
    PIKE_PROFILE_EC_DIFFERENCE_POINT,
    PIKE_PROFILE_EC_XISOG,
    PIKE_PROFILE_EC_XEVAL,
    PIKE_PROFILE_EC_ISOG_EVAL_ODD,
    PIKE_PROFILE_EC_ISOG_EVAL_EVEN,
    PIKE_PROFILE_EC_ISOG_CHAIN,
    PIKE_PROFILE_EC_ISOMORPHISM,
    PIKE_PROFILE_EC_ISO_EVAL,
    PIKE_PROFILE_EC_BASIS,
    PIKE_PROFILE_HD_THETA_CHAIN_COMPUT,
    PIKE_PROFILE_HD_THETA_CHAIN_EVAL,
    PIKE_PROFILE_HD_THETA_ISOG_COMPUT,
    PIKE_PROFILE_HD_THETA_ISOG_EVAL,
    PIKE_PROFILE_HD_GLUING,
    PIKE_PROFILE_HD_SPLITTING,
    PIKE_PROFILE_ID2ISO,
    PIKE_PROFILE_ID2ISO_ENDOMORPHISM,
    PIKE_PROFILE_KLPT_REPRESENT_INTEGER,
    PIKE_PROFILE_PIKE_KEYGEN,
    PIKE_PROFILE_PIKE_ENCRYPT,
    PIKE_PROFILE_PIKE_DECRYPT,
    PIKE_PROFILE_PIKE_ENCAPS,
    PIKE_PROFILE_PIKE_DECAPS,
    PIKE_PROFILE_PIKE_CT_ENCODE,
    PIKE_PROFILE_PIKE_CT_DECODE,
    PIKE_PROFILE_PIKE_PK_ENCODE,
    PIKE_PROFILE_PIKE_PK_DECODE,
    PIKE_PROFILE_PIKE_SK_ENCODE,
    PIKE_PROFILE_PIKE_SK_DECODE,
    PIKE_PROFILE_COMMON_XOF,
    PIKE_PROFILE_COMMON_XOF_STREAM_INIT,
    PIKE_PROFILE_COMMON_XOF_STREAM_SQUEEZE,
    PIKE_PROFILE_COMMON_HASH_256,
    PIKE_PROFILE_COMMON_RANDOMBYTES,
    PIKE_PROFILE_EVENT_COUNT
} pike_profile_event_t;

typedef struct {
    uint64_t calls;
    uint64_t cycles;
} pike_profile_entry_t;

#if defined(PIKE_PROFILE_ENABLED) && PIKE_PROFILE_ENABLED

extern pike_profile_entry_t
    pike_profile_table[PIKE_PROFILE_PHASE_COUNT][PIKE_PROFILE_EVENT_COUNT];
extern pike_profile_phase_t pike_profile_current_phase;

uint64_t pike_profile_cycles(void);
void pike_profile_reset(void);
void pike_profile_set_phase(pike_profile_phase_t phase);
void pike_profile_dump(FILE *out, int cycle_limit);

static inline void
pike_profile_count_event(pike_profile_event_t event)
{
    pike_profile_table[pike_profile_current_phase][event].calls++;
}

static inline void
pike_profile_add_cycles(pike_profile_event_t event, uint64_t cycles)
{
    pike_profile_table[pike_profile_current_phase][event].cycles += cycles;
}

static inline void
pike_profile_record_phase_cycles(uint64_t cycles)
{
    pike_profile_table[pike_profile_current_phase][PIKE_PROFILE_PHASE_TOTAL].calls++;
    pike_profile_table[pike_profile_current_phase][PIKE_PROFILE_PHASE_TOTAL].cycles += cycles;
}

#define PIKE_PROFILE_COUNT(event) pike_profile_count_event(event)
#define PIKE_PROFILE_START(var, event)                                                            \
    uint64_t var = pike_profile_cycles();                                                         \
    pike_profile_count_event(event)
#define PIKE_PROFILE_STOP(var, event)                                                             \
    pike_profile_add_cycles(event, pike_profile_cycles() - (var))
#define PIKE_PROFILE_SET_PHASE(phase) pike_profile_set_phase(phase)
#define PIKE_PROFILE_RECORD_PHASE(cycles) pike_profile_record_phase_cycles(cycles)
#define PIKE_PROFILE_RESET() pike_profile_reset()
#define PIKE_PROFILE_DUMP(out, limit) pike_profile_dump((out), (limit))

#else

#define PIKE_PROFILE_COUNT(event) ((void)0)
#define PIKE_PROFILE_START(var, event) ((void)0)
#define PIKE_PROFILE_STOP(var, event) ((void)0)
#define PIKE_PROFILE_SET_PHASE(phase) ((void)0)
#define PIKE_PROFILE_RECORD_PHASE(cycles) ((void)0)
#define PIKE_PROFILE_RESET() ((void)0)
#define PIKE_PROFILE_DUMP(out, limit) ((void)0)

#endif

#endif
