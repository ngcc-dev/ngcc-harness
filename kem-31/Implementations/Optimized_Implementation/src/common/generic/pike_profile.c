#include <pike_profile.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#if !(defined(TARGET_AMD64) || defined(TARGET_X86) || defined(TARGET_S390X))
#include <time.h>
#endif

pike_profile_entry_t pike_profile_table[PIKE_PROFILE_PHASE_COUNT][PIKE_PROFILE_EVENT_COUNT];
pike_profile_phase_t pike_profile_current_phase = PIKE_PROFILE_PHASE_NONE;

static const char *const phase_names[PIKE_PROFILE_PHASE_COUNT] = {
    "unscoped",
    "keygen",
    "encaps",
    "decaps",
    "encrypt",
    "decrypt",
    "encode_decode",
};

static const char *const event_names[PIKE_PROFILE_EVENT_COUNT] = {
    "phase.total",
    "fp.add",
    "fp.sub",
    "fp.half",
    "fp.mul",
    "fp.sqr",
    "fp.inv",
    "fp.encode",
    "fp.decode",
    "fp.decode_reduce",
    "fp.neg",
    "fp.copy",
    "fp.set_zero",
    "fp.set_one",
    "fp.set_small",
    "fp.tomont",
    "fp.frommont",
    "fp.mont_setone",
    "fp.is_equal",
    "fp.is_zero",
    "fp.is_square",
    "fp.sqrt",
    "fp.select",
    "fp.cswap",
    "fp2.add",
    "fp2.sub",
    "fp2.half",
    "fp2.mul",
    "fp2.sqr",
    "fp2.inv",
    "fp2.encode",
    "fp2.decode",
    "ec.xDBL",
    "ec.xADD",
    "ec.xDBLADD",
    "ec.xMUL",
    "ec.ec_mul",
    "ec.xDBLMUL",
    "ec.DBL",
    "ec.ADD",
    "ec.lift_point",
    "ec.difference_point",
    "ec.xisog",
    "ec.xeval",
    "ec.eval_odd",
    "ec.eval_even",
    "ec.isog_chain",
    "ec.isomorphism",
    "ec.iso_eval",
    "ec.basis",
    "hd.theta_chain_comput",
    "hd.theta_chain_eval",
    "hd.theta_isog_comput",
    "hd.theta_isog_eval",
    "hd.gluing",
    "hd.splitting",
    "id2iso",
    "id2iso.endomorphism",
    "klpt.represent_integer",
    "pike.keygen",
    "pike.encrypt",
    "pike.decrypt",
    "pike.encaps",
    "pike.decaps",
    "pike.ct_encode",
    "pike.ct_decode",
    "pike.pk_encode",
    "pike.pk_decode",
    "pike.sk_encode",
    "pike.sk_decode",
    "common.xof",
    "common.xof_stream_init",
    "common.xof_stream_squeeze",
    "common.hash_256",
    "common.randombytes",
};

static const pike_profile_entry_t *sort_phase;

uint64_t
pike_profile_cycles(void)
{
#if defined(TARGET_AMD64) || defined(TARGET_X86)
    unsigned int hi, lo;

    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)lo) | (((uint64_t)hi) << 32);
#elif defined(TARGET_S390X)
    uint64_t tod;

    asm volatile("stckf %0\n" : "=Q"(tod) : : "cc");
    return tod * 1000 / 4096;
#else
    struct timespec time;

    clock_gettime(CLOCK_REALTIME, &time);
    return (uint64_t)(time.tv_sec * 1000000000ULL + time.tv_nsec);
#endif
}

void
pike_profile_reset(void)
{
    memset(pike_profile_table, 0, sizeof(pike_profile_table));
    pike_profile_current_phase = PIKE_PROFILE_PHASE_NONE;
}

void
pike_profile_set_phase(pike_profile_phase_t phase)
{
    if ((unsigned int)phase >= PIKE_PROFILE_PHASE_COUNT) {
        pike_profile_current_phase = PIKE_PROFILE_PHASE_NONE;
        return;
    }
    pike_profile_current_phase = phase;
}

static int
event_has_data(const pike_profile_entry_t *phase, int event)
{
    return phase[event].calls != 0 || phase[event].cycles != 0;
}

static int
cmp_events_desc(const void *lhs, const void *rhs)
{
    int a = *(const int *)lhs;
    int b = *(const int *)rhs;

    if (sort_phase[a].cycles < sort_phase[b].cycles) {
        return 1;
    }
    if (sort_phase[a].cycles > sort_phase[b].cycles) {
        return -1;
    }
    if (sort_phase[a].calls < sort_phase[b].calls) {
        return 1;
    }
    if (sort_phase[a].calls > sort_phase[b].calls) {
        return -1;
    }
    return a - b;
}

static uint64_t
sum_event_calls(const pike_profile_entry_t *phase, const int *events, size_t count)
{
    uint64_t calls = 0;

    for (size_t i = 0; i < count; i++) {
        calls += phase[events[i]].calls;
    }

    return calls;
}

static uint64_t
sum_event_cycles(const pike_profile_entry_t *phase, const int *events, size_t count)
{
    uint64_t cycles = 0;

    for (size_t i = 0; i < count; i++) {
        cycles += phase[events[i]].cycles;
    }

    return cycles;
}

static void
print_field_counts(FILE *out, const pike_profile_entry_t *phase, uint64_t denom)
{
    static const int fp_fields[] = {
        PIKE_PROFILE_FP_ADD,          PIKE_PROFILE_FP_SUB,
        PIKE_PROFILE_FP_HALF,         PIKE_PROFILE_FP_MUL,
        PIKE_PROFILE_FP_SQR,          PIKE_PROFILE_FP_INV,
        PIKE_PROFILE_FP_ENCODE,       PIKE_PROFILE_FP_DECODE,
        PIKE_PROFILE_FP_DECODE_REDUCE, PIKE_PROFILE_FP_NEG,
        PIKE_PROFILE_FP_COPY,         PIKE_PROFILE_FP_SET_ZERO,
        PIKE_PROFILE_FP_SET_ONE,      PIKE_PROFILE_FP_SET_SMALL,
        PIKE_PROFILE_FP_TOMONT,       PIKE_PROFILE_FP_FROMMONT,
        PIKE_PROFILE_FP_MONT_SETONE,  PIKE_PROFILE_FP_IS_EQUAL,
        PIKE_PROFILE_FP_IS_ZERO,      PIKE_PROFILE_FP_IS_SQUARE,
        PIKE_PROFILE_FP_SQRT,         PIKE_PROFILE_FP_SELECT,
        PIKE_PROFILE_FP_CSWAP,
    };
    static const int fp2_fields[] = {
        PIKE_PROFILE_FP2_ADD,    PIKE_PROFILE_FP2_SUB,
        PIKE_PROFILE_FP2_HALF,   PIKE_PROFILE_FP2_MUL,
        PIKE_PROFILE_FP2_SQR,    PIKE_PROFILE_FP2_INV,
        PIKE_PROFILE_FP2_ENCODE, PIKE_PROFILE_FP2_DECODE,
    };
    uint64_t fp_calls = sum_event_calls(
        phase, fp_fields, sizeof(fp_fields) / sizeof(fp_fields[0]));
    uint64_t fp_cycles = sum_event_cycles(
        phase, fp_fields, sizeof(fp_fields) / sizeof(fp_fields[0]));
    uint64_t fp2_calls = sum_event_calls(
        phase, fp2_fields, sizeof(fp2_fields) / sizeof(fp2_fields[0]));
    uint64_t fp2_cycles = sum_event_cycles(
        phase, fp2_fields, sizeof(fp2_fields) / sizeof(fp2_fields[0]));
    double fp_pct = denom == 0 ? 0.0 : 100.0 * (double)fp_cycles / (double)denom;
    double fp2_pct = denom == 0 ? 0.0 : 100.0 * (double)fp2_cycles / (double)denom;
    int any = 0;

    if (fp_calls != 0 || fp_cycles != 0) {
        fprintf(out,
                "  field.fp_total              calls=%10" PRIu64
                " incl_cycles=%14" PRIu64 " pct=%7.2f\n",
                fp_calls,
                fp_cycles,
                fp_pct);
    }
    if (fp2_calls != 0 || fp2_cycles != 0) {
        fprintf(out,
                "  field.fp2_markers           calls=%10" PRIu64
                " incl_cycles=%14" PRIu64 " pct=%7.2f\n",
                fp2_calls,
                fp2_cycles,
                fp2_pct);
    }

    for (size_t i = 0; i < sizeof(fp_fields) / sizeof(fp_fields[0]); i++) {
        any |= phase[fp_fields[i]].calls != 0;
    }
    for (size_t i = 0; i < sizeof(fp2_fields) / sizeof(fp2_fields[0]); i++) {
        any |= phase[fp2_fields[i]].calls != 0;
    }
    if (!any) {
        return;
    }

    fprintf(out, "  field calls:");
    for (size_t i = 0; i < sizeof(fp_fields) / sizeof(fp_fields[0]); i++) {
        int event = fp_fields[i];
        if (phase[event].calls != 0) {
            fprintf(out, " %s=%" PRIu64, event_names[event], phase[event].calls);
        }
    }
    for (size_t i = 0; i < sizeof(fp2_fields) / sizeof(fp2_fields[0]); i++) {
        int event = fp2_fields[i];
        if (phase[event].calls != 0) {
            fprintf(out, " %s=%" PRIu64, event_names[event], phase[event].calls);
        }
    }
    fprintf(out, "\n");
}

void
pike_profile_dump(FILE *out, int cycle_limit)
{
    int events[PIKE_PROFILE_EVENT_COUNT - 1];

    if (cycle_limit <= 0) {
        cycle_limit = 20;
    }

    fprintf(out, "\n[pike-profile] inclusive cycle counters; field mul/add/sub/sqr/half are counted only\n");
    for (int phase_idx = 0; phase_idx < PIKE_PROFILE_PHASE_COUNT; phase_idx++) {
        const pike_profile_entry_t *phase = pike_profile_table[phase_idx];
        uint64_t denom = phase[PIKE_PROFILE_PHASE_TOTAL].cycles;
        int event_count = 0;

        if (phase_idx == PIKE_PROFILE_PHASE_NONE &&
            !event_has_data(phase, PIKE_PROFILE_PHASE_TOTAL)) {
            continue;
        }

        if (!event_has_data(phase, PIKE_PROFILE_PHASE_TOTAL)) {
            int any = 0;
            for (int event = 1; event < PIKE_PROFILE_EVENT_COUNT; event++) {
                any |= event_has_data(phase, event);
            }
            if (!any) {
                continue;
            }
        }

        fprintf(out,
                "[pike-profile] phase=%s calls=%" PRIu64 " total_cycles=%" PRIu64 "\n",
                phase_names[phase_idx],
                phase[PIKE_PROFILE_PHASE_TOTAL].calls,
                denom);

        for (int event = 1; event < PIKE_PROFILE_EVENT_COUNT; event++) {
            if (phase[event].cycles != 0) {
                events[event_count++] = event;
            }
        }

        sort_phase = phase;
        qsort(events, event_count, sizeof(events[0]), cmp_events_desc);
        sort_phase = NULL;

        for (int i = 0; i < event_count && i < cycle_limit; i++) {
            int event = events[i];
            uint64_t calls = phase[event].calls;
            uint64_t cycles = phase[event].cycles;
            double avg = calls == 0 ? 0.0 : (double)cycles / (double)calls;
            double pct = denom == 0 ? 0.0 : 100.0 * (double)cycles / (double)denom;

            fprintf(out,
                    "  %-28s calls=%10" PRIu64 " incl_cycles=%14" PRIu64
                    " avg=%10.1f pct=%7.2f\n",
                    event_names[event],
                    calls,
                    cycles,
                    avg,
                    pct);
        }
        print_field_counts(out, phase, denom);
    }
}
