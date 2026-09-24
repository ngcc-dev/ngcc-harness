/*
 * DKE KEM — QEMU KAT + 性能测试 harness
 *
 * 无 newlib / rdimon；输出经 semihosting SYS_WRITE0；
 * 计时经 SysTick 24-bit 下行计数器（QEMU netduinoplus2 支持）；
 * 结束经 semihosting SYS_EXIT（不用 while(1)，QEMU 正常退出）。
 *
 * 编译宏（外部 -D 传入）：
 *   DKE_MODE           = 128 / 256 / 512
 *   DKE_USE_CORTEX_M4_BARRETT / DKE_USE_CORTEX_M4 / DKE_USE_CORTEX_M4_PLANTARD
 *   DKE_USE_MATACC     （可选，仅 Plantard + 128/256）
 *   DKE_HASH           = 0 (SM3)  / 2 (ML-KEM-suite)
 *   DKE_RANDOM         = 0  (derand API，无 RNG 依赖)
 */
#include <stdint.h>
#include <string.h>
#include "parameters.h"
#include "dkecca.h"

/* ------------------------------------------------------------------ SysTick */
#define SYST_CSR (*(volatile uint32_t*)0xE000E010)
#define SYST_RVR (*(volatile uint32_t*)0xE000E014)
#define SYST_CVR (*(volatile uint32_t*)0xE000E018)

static void syst_init(void) { SYST_RVR = 0xFFFFFF; SYST_CVR = 0; SYST_CSR = 5; }

/* ----------------------------------------- semihosting helpers (no newlib) */
__attribute__((noinline))
static void sh_puts(const char *s) {
    __asm__ volatile (
        "mov r0, #0x04\n\t"
        "mov r1, %0\n\t"
        "bkpt #0xAB\n\t"
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
        "mov r0, #0x18\n\t"
        "ldr r1, =0x20026\n\t"
        "bkpt #0xAB\n\t"
        : : : "r0","r1");
    while(1);
}

/* ---------------------------------------------------------------- 后端名称 */
static const char *backend_str(void) {
#if   defined(DKE_USE_CORTEX_M4_BARRETT)
    return "Barrett";
#elif defined(DKE_USE_CORTEX_M4_PLANTARD)
    return "Plantard";
#elif defined(DKE_USE_CORTEX_M4)
    return "Montgomery";
#else
    return "Generic-C";
#endif
}

/* ------------------------------------------------------------------ 主程序 */
#ifndef NTESTS
#define NTESTS 5
#endif

static uint8_t pk  [DKE_PKBYTES];
static uint8_t sk  [DKE_SKBYTES];
static uint8_t ct  [DKE_CTBYTES];
static uint8_t ss_enc[DKE_SSBYTES];
static uint8_t ss_dec[DKE_SSBYTES];
static uint8_t coins_kg [DKE_SEEDBYTES * 2];
static uint8_t coins_enc[DKE_SEEDBYTES + DKE_N / 8];

int main(void) {
    uint32_t kg_min = 0xFFFFFF, en_min = 0xFFFFFF, de_min = 0xFFFFFF;
    int ok = 1;

    syst_init();

    sh_puts("  DKE-");
#if   DKE_MODE == 128
    sh_puts("128");
#elif DKE_MODE == 256
    sh_puts("256");
#else
    sh_puts("512");
#endif
    sh_puts("  ");
    sh_puts(backend_str());
    sh_puts("  ");
#if DKE_HASH == 0
    sh_puts("SM3");
#else
    sh_puts("ML-KEM-suite");
#endif
    sh_puts("\n");

    for (int i = 0; i < NTESTS; i++) {
        memset(coins_kg,  0x11 + i, sizeof(coins_kg));
        memset(coins_enc, 0x22 + i, sizeof(coins_enc));

        uint32_t b, a;

        b = SYST_CVR;
        DKE_CCA_keygen_derand(pk, sk, coins_kg);
        a = SYST_CVR; { uint32_t d = (b-a)&0xFFFFFF; if(d<kg_min) kg_min=d; }

        b = SYST_CVR;
        DKE_CCA_enc_derand(ct, ss_enc, pk, coins_enc);
        a = SYST_CVR; { uint32_t d = (b-a)&0xFFFFFF; if(d<en_min) en_min=d; }

        b = SYST_CVR;
        DKE_CCA_dec(ss_dec, sk, ct);
        a = SYST_CVR; { uint32_t d = (b-a)&0xFFFFFF; if(d<de_min) de_min=d; }

        if (memcmp(ss_enc, ss_dec, DKE_SSBYTES) != 0) {
            sh_puts("FAIL\n");
            ok = 0; break;
        }
    }

    if (ok) {
        print_u32("keygen:", kg_min);
        print_u32("encaps:", en_min);
        print_u32("decaps:", de_min);
        sh_puts("PASS\n");
    }

    sh_exit(ok ? 0 : 1);
    return ok ? 0 : 1;
}
