#ifndef MEM_STAT_H
#define MEM_STAT_H

#include <stdint.h>

/* ── Dynamic memory queries ───────────────────────────────────── */

uint64_t ngcc_mem_current_rss_bytes(void);
uint64_t ngcc_mem_current_vmsize_bytes(void);
uint64_t ngcc_mem_peak_rss_bytes(void);
uint64_t ngcc_mem_vm_peak_bytes(void);

#endif
