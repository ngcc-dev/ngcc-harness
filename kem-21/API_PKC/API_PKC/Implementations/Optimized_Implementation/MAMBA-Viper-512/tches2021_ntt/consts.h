#ifndef CONSTS_H
#define CONSTS_H

#include "params.h"

#if NTT_N == 256
#include "consts256.h"
#elif NTT_N == 512
#include "consts512.h"
#elif NTT_N == 1024
#include "consts1024.h"
#elif NTT_N == 1536
#include "consts1536.h"
#elif NTT_N == 1728
#include "consts1728.h"
#endif

#endif
