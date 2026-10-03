/* Counts xof_squeeze calls made by the UNMODIFIED rsdp.c (renamed via -D), no decisions changed. */
#include "hash.h"
unsigned long g_sq = 0;
void xof_squeeze_counted(xof_ctx_t *c, uint8_t *o, size_t n) { g_sq++; xof_squeeze(c, o, n); }

