/*
 * rans.c -- static byte-renormalized rANS codec for SHUTTLE.
 *
 * Byte-exact to the Python golden model tools/rans.py.  32-bit state,
 * L = 2^23, 8-bit renorm, prob_bits = 10, RANS_N = 2 interleaved streams.
 * The flat symbol sequence is Q0 then Qs then hint (logical order); symbol
 * t is coded on its power-of-two interleave state s = t & MASK.  Encode
 * pushes in REVERSE t (so decode pulls forward t), all states sharing one
 * backward-written byte stream.  N=2 hides the ~5-cyc reciprocal-multiply
 * latency at the cost of only 4 extra flush bytes (negligible vs
 * RANS_RESERVED_BYTES); see rans.h for the engine rationale.
 *
 * CONSTANT-TIME: this operates on the PUBLIC, post-signing signature
 * data, so the renorm `while` loops and `pos` bounds are not a CT
 * violation.
 */
#include "rans.h"

#include <stdint.h>
#include <string.h>

#define PB RANS_PROB_BITS /* 10 */
#define PSCALE (1u << PB)
#define RN RANS_INTERLEAVED_STREAMS
#define RMASK RANS_INTERLEAVED_MASK

static inline int rans_state_in_range(uint32_t x)
{
    return x >= RANS_L && x < (RANS_L << 8);
}

/* one encode step (push): updates *x, writes renorm bytes to out[--*pos].
 * The quotient/remainder is replaced by a multiply-by-reciprocal (ryg rANS
 * / Granlund-Montgomery; rcp/rsh/bias are generated + verified in
 * tools/gen_rans_tables.py), bit-exact for the encoder's x range, so no
 * hardware divide touches the (public) signature data and the byte stream
 * -- hence the KAT -- is unchanged. */
static inline void enc_put(uint32_t *x, uint8_t *out, size_t *pos,
                           uint32_t freq, uint32_t rcp, uint32_t rsh,
                           uint32_t bias)
{
    uint32_t x_max = ((RANS_L >> PB) << 8) * freq;
    while (*x >= x_max) {
        out[--(*pos)] = (uint8_t)(*x & 0xff);
        *x >>= 8;
    }
    uint32_t q = (uint32_t)(((uint64_t)*x * rcp) >> 32) >> rsh;
    *x = *x + bias + q * (PSCALE - freq);
}

/* one encode step keyed by a contiguous-alphabet table on a single state
 * XS; returns -2 if the symbol is out of the modelled support (Sign
 * restarts) or the backward write cursor `pos` would underflow.
 * `out`/`pos` are the caller's shared backward byte cursor. */
#define ENC_STEP(TBL, XS, SYM)                                         \
    do {                                                               \
        int slot_ = (int)((SYM)-TBL##_LO);                             \
        if (slot_ < 0 || slot_ >= TBL##_N)                             \
            return -2;                                                 \
        if (pos < (size_t)(4 * RN + 4))                                \
            return -2;                                                 \
        enc_put(&(XS), out, &pos, TBL##_FREQ[slot_], TBL##_RCP[slot_], \
                TBL##_RSH[slot_], TBL##_BIAS[slot_]);                  \
    } while (0)

/* Encode `cnt` consecutive symbols of ONE model (table TBL) keyed by SRC,
 * continuing the global interleave so that flat index t = T0 + i selects
 * stream s = t & RMASK.  Symbols are PUSHED IN REVERSE t (i = cnt-1 .. 0),
 * exactly matching the original single reverse loop, so the backward byte
 * stream -- and hence the KAT -- is unchanged. */
#if RANS_INTERLEAVED_STREAMS == 2
/* RN==2 specialization: the two states are two independent dependency
 * chains, so holding them in named locals x0/x1 and stepping them pairwise
 * lets the out-of-order core overlap the reciprocal-multiply latency of one
 * chain with the other.  Reverse order, high t first: the largest t in the
 * run is (T0+cnt-1).  A leading symbol whose stream is x1 (its t is odd) is
 * peeled so the remaining top t is EVEN; the rest then step in (x0,x1) pairs
 * (high-t even -> x0, then low-t odd -> x1), with a final lone bottom symbol
 * (its t is even -> x0).  The per-symbol stream/order match the fused loop. */
#    define ENC_RUN(TBL, SRC, CNT, T0)                   \
        do {                                             \
            size_t j_ = (CNT);                           \
            if (j_ > 0 && (((T0) + j_ - 1) & 1u) != 0) { \
                j_--;                                    \
                ENC_STEP(TBL, x1, (SRC)[j_]);            \
            }                                            \
            while (j_ >= 2) {                            \
                ENC_STEP(TBL, x0, (SRC)[j_ - 1]);        \
                ENC_STEP(TBL, x1, (SRC)[j_ - 2]);        \
                j_ -= 2;                                 \
            }                                            \
            if (j_ > 0)                                  \
                ENC_STEP(TBL, x0, (SRC)[j_ - 1]);        \
        } while (0)
#else
#    define ENC_RUN(TBL, SRC, CNT, T0)                         \
        do {                                                   \
            for (size_t j_ = (CNT); j_-- > 0;) {               \
                unsigned s_ = (unsigned)(((T0) + j_) & RMASK); \
                ENC_STEP(TBL, x[s_], (SRC)[j_]);               \
            }                                                  \
        } while (0)
#endif

int shuttle_rans_encode(uint8_t *out, size_t *out_len, size_t cap,
                        const int32_t *q0, const int32_t *qs,
                        const int32_t *h, size_t nq0, size_t nqs,
                        size_t nh)
{
    uint32_t x[RN];
    for (int s = 0; s < RN; s++)
        x[s] = RANS_L;
    size_t pos = cap; /* write backwards */
#if RANS_INTERLEAVED_STREAMS == 2
    uint32_t x0 = x[0], x1 = x[1];
#endif

    /* flat S[t]: q0[t] (t<nq0, Q0) then qs[t-nq0] (Qs) then h[...] (HINT).
     * Pushing in REVERSE t visits the three models back-to-back in reverse
     * order (HINT, then Qs, then Q0); splitting the single reverse loop
     * into three contiguous runs hoists the model-selection branch out of
     * the inner step.  Each run continues the global interleave at its
     * start index, so the per-symbol stream/model/order/guard are
     * identical to the fused loop -- the encoded bytes are unchanged. */
    ENC_RUN(RANS_HINT, h, nh, nq0 + nqs);
    ENC_RUN(RANS_QS, qs, nqs, nq0);
    ENC_RUN(RANS_Q0, q0, nq0, (size_t)0);

#if RANS_INTERLEAVED_STREAMS == 2
    x[0] = x0;
    x[1] = x1;
#endif
    /* flush all RN states, state 0 first (lands last in the stream, so
     * decode reads state RN-1 first). */
    for (int s = 0; s < RN; s++)
        for (int b = 0; b < 4; b++) {
            if (pos == 0)
                return -2;
            out[--pos] = (uint8_t)(x[s] & 0xff);
            x[s] >>= 8;
        }
    size_t used = cap - pos;
    memmove(out, out + pos, used);
    *out_len = used;
    return 0;
}

/* Per-`val` decode entry: collapses the chained SLOT[val] -> FREQ[slot] /
 * CDF[slot] indirection of the const tables into one lookup.  For a given
 * val in [0, PSCALE), let slot = SLOT[val]; then
 *     sym  = LO + slot                (decoded symbol value)
 *     freq = FREQ[slot]               (in [1, PSCALE])
 *     bias = val - CDF[slot]          (in [0, freq), so non-negative)
 * and the state update is  x = freq * (x >> PB) + bias.  Packing freq and
 * bias into one 32-bit word (each < PSCALE <= 2^10 fits in 16 bits) plus a
 * separate symbol word makes the inner step a single contiguous 8-byte
 * load.  The values are IDENTICAL to those produced by the chained lookup,
 * so the decoded output -- and hence the KAT -- is unchanged. */
typedef struct {
    uint32_t fb; /* freq << 16 | bias */
    int32_t sym; /* LO + slot         */
} rans_dsym;

/* Build the per-val table for one model from its const SLOT/FREQ/CDF/LO.
 */
static void rans_build_dsym(rans_dsym *d, const uint8_t *slot,
                            const uint16_t *freq, const uint16_t *cdf,
                            int lo)
{
    for (uint32_t v = 0; v < PSCALE; v++) {
        unsigned s = slot[v];
        d[v].fb = ((uint32_t)freq[s] << 16) | (uint32_t)(v - cdf[s]);
        d[v].sym = lo + (int)s;
    }
}

/* Lazily build all three model tables exactly once.  The content is a pure
 * function of the compile-time const tables (deterministic, idempotent),
 * so the unguarded first-build race is benign for the public verify path.
 */
static rans_dsym rans_q0_dsym[PSCALE];
static rans_dsym rans_qs_dsym[PSCALE];
static rans_dsym rans_hint_dsym[PSCALE];
static int rans_dsym_ready = 0;

static void rans_init_dsym(void)
{
    rans_build_dsym(rans_q0_dsym, RANS_Q0_SLOT, RANS_Q0_FREQ, RANS_Q0_CDF,
                    RANS_Q0_LO);
    rans_build_dsym(rans_qs_dsym, RANS_QS_SLOT, RANS_QS_FREQ, RANS_QS_CDF,
                    RANS_QS_LO);
    rans_build_dsym(rans_hint_dsym, RANS_HINT_SLOT, RANS_HINT_FREQ,
                    RANS_HINT_CDF, RANS_HINT_LO);
    rans_dsym_ready = 1;
}

/* one decode step on a single state XS keyed by a packed per-val table:
 * emit the symbol, advance the state with one table load, then
 * byte-renorm. `bp`/`in`/`in_len` are the caller's shared backward byte
 * cursor; an empty stream short-circuits to -1. */
#define DEC_STEP(DTBL, DST, XS)                                  \
    do {                                                         \
        uint32_t val_ = (XS) & (PSCALE - 1);                     \
        rans_dsym e_ = (DTBL)[val_];                             \
        (DST) = e_.sym;                                          \
        (XS) = (e_.fb >> 16) * ((XS) >> PB) + (e_.fb & 0xffffu); \
        while ((XS) < RANS_L) {                                  \
            if (bp >= in_len)                                    \
                return -1;                                       \
            (XS) = ((XS) << 8) | in[bp++];                       \
        }                                                        \
    } while (0)

/* Decode `cnt` consecutive symbols of ONE model (packed table DTBL) into
 * DST[0..cnt), continuing the global interleave at start index `t0`.  The
 * interleave state s = (t0+i) & RMASK selects which stream advances. Bytes
 * are consumed strictly in increasing t -- identical to a per-symbol loop
 * -- so the decoded output is unchanged. */
#if RANS_INTERLEAVED_STREAMS == 2
/* RN==2 specialization: the two states are two independent dependency
 * chains, so holding them in named locals x0/x1 and stepping them pairwise
 * lets the out-of-order core overlap the latency of one chain with the
 * other.  A leading odd-phase symbol (when t0 is odd) is peeled onto x1.
 */
#    define DEC_RUN(DTBL, DST, CNT, T0)            \
        do {                                       \
            size_t i_ = 0;                         \
            if (((T0)&1u) != 0 && i_ < (CNT)) {    \
                DEC_STEP(DTBL, (DST)[i_], x1);     \
                i_++;                              \
            }                                      \
            for (; i_ + 1 < (CNT); i_ += 2) {      \
                DEC_STEP(DTBL, (DST)[i_], x0);     \
                DEC_STEP(DTBL, (DST)[i_ + 1], x1); \
            }                                      \
            if (i_ < (CNT))                        \
                DEC_STEP(DTBL, (DST)[i_], x0);     \
        } while (0)
#else
#    define DEC_RUN(DTBL, DST, CNT, T0)             \
        do {                                        \
            for (size_t i_ = 0; i_ < (CNT); i_++) { \
                unsigned s_ = ((T0) + i_) & RMASK;  \
                DEC_STEP(DTBL, (DST)[i_], x[s_]);   \
            }                                       \
        } while (0)
#endif

int shuttle_rans_decode(int32_t *q0, int32_t *qs, int32_t *h, size_t nq0,
                        size_t nqs, size_t nh, const uint8_t *in,
                        size_t in_len)
{
    if (in_len < (size_t)(4 * RN))
        return -1;
    if (!rans_dsym_ready)
        rans_init_dsym();
    uint32_t x[RN];
    size_t bp = 0;
    /* init states in reverse flush order: stream front is state RN-1, ...,
     * then state 0 (mirror of the encoder's flush).  Range-check each (the
     * canonical initial-state check). */
    for (int s = RN - 1; s >= 0; s--) {
        uint32_t v = 0;
        for (int b = 0; b < 4; b++)
            v = (v << 8) | in[bp++];
        x[s] = v;
        if (!rans_state_in_range(x[s]))
            return -1;
    }
#if RANS_INTERLEAVED_STREAMS == 2
    uint32_t x0 = x[0], x1 = x[1];
#endif

    /* The flat symbol stream is the three models back-to-back (Q0, then
     * Qs, then HINT); splitting the single dispatch loop into three
     * contiguous runs hoists the model-selection branch out of the inner
     * step.  val in [0, PSCALE) maps to a valid slot (CDF[0]=0,
     * CDF[N]=PSCALE: no hole); the decoded SYMBOL VALUE is range-checked
     * by the caller (per-block support + hint range-check in unpack_sig).
     */
    DEC_RUN(rans_q0_dsym, q0, nq0, (size_t)0);
    DEC_RUN(rans_qs_dsym, qs, nqs, nq0);
    DEC_RUN(rans_hint_dsym, h, nh, nq0 + nqs);

#if RANS_INTERLEAVED_STREAMS == 2
    x[0] = x0;
    x[1] = x1;
#endif
    /* Canonical stream check: all bytes consumed and all interleaved
     * states rewind to the encoder's initial state L. */
    if (bp != in_len)
        return -1;
    for (int s = 0; s < RN; s++)
        if (x[s] != RANS_L)
            return -1;
    return 0;
}
