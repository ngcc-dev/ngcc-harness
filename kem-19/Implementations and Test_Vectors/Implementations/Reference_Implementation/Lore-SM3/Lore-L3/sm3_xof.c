#include "sm3_xof.h"
#include "sm3.h"

#include <stdlib.h>
#include <string.h>

enum {
    LORE_MODE_SM3_XOF128 = 0x11,
    LORE_MODE_SM3_XOF256 = 0x12
};

static void state_reset(sm3_xof_state *state, uint8_t mode)
{
    memset(state, 0, sizeof(*state));
    state->mode = mode;
}

static void state_absorb(sm3_xof_state *state, const uint8_t *in, size_t inlen)
{
    if (state->overflow || inlen == 0) {
        return;
    }

    if (state->inlen + inlen > sizeof(state->input)) {
        state->overflow = 1;
        return;
    }

    memcpy(state->input + state->inlen, in, inlen);
    state->inlen += inlen;
}

static int tagged_xof(uint8_t *out,
                      size_t outlen,
                      uint8_t tag,
                      const uint8_t *in,
                      size_t inlen)
{
    size_t tagged_len = inlen + 1;
    uint8_t *buf = (uint8_t *)malloc(tagged_len);

    if (buf == NULL) {
        return -1;
    }

    buf[0] = tag;
    if (inlen > 0) {
        memcpy(buf + 1, in, inlen);
    }

    sm3_kdf(out, outlen, buf, tagged_len);

    free(buf);
    return 0;
}

static void state_squeeze(uint8_t *out, size_t outlen, sm3_xof_state *state)
{
    size_t total;
    uint8_t *tmp;

    if (outlen == 0) {
        return;
    }

    if (state->overflow) {
        memset(out, 0, outlen);
        return;
    }

    state->finalized = 1;
    total = state->outpos + outlen;
    tmp = (uint8_t *)malloc(total);
    if (tmp == NULL) {
        memset(out, 0, outlen);
        return;
    }

    if (tagged_xof(tmp, total, state->mode, state->input, state->inlen) != 0) {
        memset(out, 0, outlen);
        free(tmp);
        return;
    }

    memcpy(out, tmp + state->outpos, outlen);
    state->outpos += outlen;
    free(tmp);
}

void sm3_xof128_absorb_once(sm3_xof_state *state, const uint8_t *in, size_t inlen)
{
    state_reset(state, LORE_MODE_SM3_XOF128);
    state_absorb(state, in, inlen);
    state->finalized = 1;
}

void sm3_xof128_squeezeblocks(uint8_t *out, size_t nblocks, sm3_xof_state *state)
{
    state_squeeze(out, nblocks * SM3_XOF128_RATE, state);
}

void sm3_xof256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
    if (tagged_xof(out, outlen, LORE_MODE_SM3_XOF256, in, inlen) != 0) {
        memset(out, 0, outlen);
    }
}
