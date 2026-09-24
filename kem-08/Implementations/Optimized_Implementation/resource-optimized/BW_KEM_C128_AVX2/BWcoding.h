#ifndef BWKEM128_CODEC_H
#define BWKEM128_CODEC_H

#include <stdint.h>
#include "params.h"

void codec_encode(int16_t out[BWKEM128_N],
                  const uint8_t msg[BWKEM128_INDCPA_MSGBYTES]);

void codec_decode(uint8_t msg[BWKEM128_INDCPA_MSGBYTES],
                  const int16_t in[BWKEM128_N]);

#endif
