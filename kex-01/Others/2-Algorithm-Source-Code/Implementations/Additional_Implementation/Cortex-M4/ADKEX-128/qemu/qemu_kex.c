/*
 * ADKEX (authenticated key exchange) — QEMU round-trip self-test (Cortex-M4).
 *
 * Uses the fully derandomized ADKEX API (explicit coins, no RNG), mirroring the
 * DKEM-M4 harness: semihosting SYS_WRITE0 output, SysTick cycle counts, SYS_EXIT
 * to terminate QEMU. Runs the 2-pass ADKEX handshake and checks ssa == ssb.
 *
 * Compile macros (-D): ADKEX_MODE=128/256/512, DKE_MODE=<same>, DKE_HASH=0 (SM3),
 *   DKE_RANDOM=0, DKE_USE_CORTEX_M4_PLANTARD, DKE_USE_MATACC.
 */
#include <stdint.h>
#include <string.h>
#include "ADKEX_parameters.h"
#include "adkex_derand.h"

/* ------------------------------------------------------------------ SysTick */
#define SYST_CSR (*(volatile uint32_t*)0xE000E010)
#define SYST_RVR (*(volatile uint32_t*)0xE000E014)
#define SYST_CVR (*(volatile uint32_t*)0xE000E018)
static void syst_init(void) { SYST_RVR = 0xFFFFFF; SYST_CVR = 0; SYST_CSR = 5; }

/* ----------------------------------------- semihosting helpers (no newlib) */
__attribute__((noinline))
static void sh_puts(const char *s) {
    __asm__ volatile (
        "mov r0, #0x04\n\t" "mov r1, %0\n\t" "bkpt #0xAB\n\t"
        : : "r"(s) : "r0","r1","r2","r3","memory");
}
static void u32_str(uint32_t v, char *buf) {
    char tmp[11]; int i = 0;
    if (!v) { buf[0]='0'; buf[1]='\0'; return; }
    while (v) { tmp[i++] = '0' + (v % 10); v /= 10; }
    for (int j = 0; j < i; j++) buf[j] = tmp[i-1-j];
    buf[i] = '\0';
}
static void print_u32(const char *label, uint32_t val) {
    char line[80], num[12]; int i = 0;
    while (*label) line[i++] = *label++;
    line[i++] = ' ';
    u32_str(val, num);
    const char *p = num; while (*p) line[i++] = *p++;
    line[i++] = '\n'; line[i] = '\0';
    sh_puts(line);
}
static void sh_exit(int code) {
    (void)code;
    __asm__ volatile (
        "mov r0, #0x18\n\t" "ldr r1, =0x20026\n\t" "bkpt #0xAB\n\t"
        : : : "r0","r1");
    while(1);
}

/* ------------------------------------------------------------------ buffers */
#define SSB (ADKEX_SSBITS / 8)
static uint8_t pkB[ADKEX_PKBITS / 8], skB[ADKEX_SKBITS / 8];
static uint8_t m1[ADKEX_M1_BITS / 8],  m2[ADKEX_M2_BITS / 8];
static uint8_t sta[ADKEX_STA_MAX_BITS / 8], stb[ADKEX_STB_MAX_BITS / 8];
static uint8_t ssa[SSB], ssb[SSB];
static uint8_t cb[ADKEX_INIT_B_COINBITS / 8];
static uint8_t c1[ADKEX_PASS1_COINBITS / 8];
static uint8_t c2[ADKEX_PASS2_COINBITS / 8];

#ifndef NTESTS
#define NTESTS 5
#endif

int main(void) {
    uint32_t ib = 0xFFFFFF, p1 = 0xFFFFFF, p2 = 0xFFFFFF, da = 0xFFFFFF;
    int ok = 1;
    syst_init();

    sh_puts("  ADKEX-");
#if   ADKEX_MODE == 128
    sh_puts("128");
#elif ADKEX_MODE == 256
    sh_puts("256");
#else
    sh_puts("512");
#endif
    sh_puts("  Plantard  SM3\n");

    for (int i = 0; i < NTESTS; i++) {
        memset(cb, 0x11 + i, sizeof cb);
        memset(c1, 0x22 + i, sizeof c1);
        memset(c2, 0x33 + i, sizeof c2);
        uint32_t b, a;

        b = SYST_CVR; ADKEX_init_b_derand(pkB, skB, cb);
        a = SYST_CVR; { uint32_t d = (b-a)&0xFFFFFF; if (d<ib) ib=d; }

        b = SYST_CVR; ADKEX_pass1_msg_a_derand(m1, sta, pkB, c1);
        a = SYST_CVR; { uint32_t d = (b-a)&0xFFFFFF; if (d<p1) p1=d; }

        b = SYST_CVR; ADKEX_pass2_msg_b_derand(m2, stb, m1, pkB, skB, c2);
        a = SYST_CVR; { uint32_t d = (b-a)&0xFFFFFF; if (d<p2) p2=d; }

        b = SYST_CVR; ADKEX_derive_ss_a(ssa, m2, sta, pkB);
        a = SYST_CVR; { uint32_t d = (b-a)&0xFFFFFF; if (d<da) da=d; }

        ADKEX_derive_ss_b(ssb, stb);

        if (memcmp(ssa, ssb, SSB) != 0) { sh_puts("FAIL\n"); ok = 0; break; }
    }

    if (ok) {
        print_u32("init_b:", ib);
        print_u32("pass1 :", p1);
        print_u32("pass2 :", p2);
        print_u32("derive:", da);
        sh_puts("PASS\n");
    }
    sh_exit(ok ? 0 : 1);
    return ok ? 0 : 1;
}
