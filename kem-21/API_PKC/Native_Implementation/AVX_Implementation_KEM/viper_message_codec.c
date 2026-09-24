#include "viper_message_codec.h"
#include "viper_e8.h"
#include <string.h>

#if !VIPER_USE_E8_CODEC
static void viper_scalar_encode(vpoly out, const unsigned char m[VIPER_MSGBYTES]) {
  memset(out, 0, sizeof(vpoly));
  for (size_t i = 0; i < VIPER_MSG_COEFFS; i++) {
    unsigned val = 0;
    for (unsigned b = 0; b < VIPER_MSG_BITS_PER_COEFF; b++) {
      size_t bit = i * VIPER_MSG_BITS_PER_COEFF + b;
      val |= (unsigned)((m[bit >> 3] >> (bit & 7)) & 1u) << b;
    }
    out[i] = (uint16_t)(val * (VIPER_Q >> VIPER_MSG_BITS_PER_COEFF));
  }
}

static void viper_scalar_decode(unsigned char m[VIPER_MSGBYTES], const vpoly in) {
  memset(m, 0, VIPER_MSGBYTES);
  const unsigned scale = VIPER_Q >> VIPER_MSG_BITS_PER_COEFF;
  const unsigned levels = 1u << VIPER_MSG_BITS_PER_COEFF;
  for (size_t i = 0; i < VIPER_MSG_COEFFS; i++) {
    unsigned x = in[i] & VIPER_QMASK;
    unsigned val = ((x + (scale >> 1)) / scale) & (levels - 1u);
    for (unsigned b = 0; b < VIPER_MSG_BITS_PER_COEFF; b++) {
      size_t bit = i * VIPER_MSG_BITS_PER_COEFF + b;
      if (bit < VIPER_MSGBITS) m[bit >> 3] |= (unsigned char)(((val >> b) & 1u) << (bit & 7));
    }
  }
}

#endif

void viper_msg_encode(vpoly out, const unsigned char m[VIPER_MSGBYTES]) {
#if VIPER_USE_E8_CODEC
  viper_e8_encode(out, m);
#else
  viper_scalar_encode(out, m);
#endif
}

void viper_msg_decode(unsigned char m[VIPER_MSGBYTES], const vpoly in) {
#if VIPER_USE_E8_CODEC
  viper_e8_decode(m, in);
#else
  viper_scalar_decode(m, in);
#endif
}

const char *viper_message_codec_name(void) {
#if VIPER_USE_E8_CODEC
  return "E8-rectangular-coset";
#else
  return "scalar-coefficient-wise";
#endif
}
