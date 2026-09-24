#ifndef OPSSIG_REJSAMPLE_H
#define OPSSIG_REJSAMPLE_H

#include <stdint.h>
#include "params.h"

#define REJ_UNIFORM_BUFLEN \
  (4U * (unsigned int)((((uint64_t)N << 26) + Q - 1) / Q))

#if ETA == 2
#define REJ_UNIFORM_ETA_BUFLEN ((8U * N + 14U) / 15U)
#elif ETA == 3
#define REJ_UNIFORM_ETA_BUFLEN ((8U * N + 13U) / 14U)
#endif

unsigned int rej_uniform_avx(int32_t * restrict r,
                             const uint8_t buf[REJ_UNIFORM_BUFLEN + 8]);
unsigned int rej_eta_avx(int32_t * restrict r,
                         const uint8_t buf[REJ_UNIFORM_ETA_BUFLEN]);

#endif
