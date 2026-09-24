#define _POSIX_C_SOURCE 200809L

#include "hygon_cis_sm4.h"

#include <setjmp.h>
#include <signal.h>
#include <string.h>

static sigjmp_buf hygon_cis_sigill_jmp;
static volatile sig_atomic_t hygon_cis_sigill_active;

static void hygon_cis_sigill_handler(int signum) {
  if (hygon_cis_sigill_active) {
    siglongjmp(hygon_cis_sigill_jmp, 1);
  }

  signal(signum, SIG_DFL);
  raise(signum);
}

static int hygon_cis_sm4_run_selftest(void) {
  static const uint8_t key[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                                  0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};
  static const uint8_t pt[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                                 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};
  static const uint8_t expected[16] = {0x68, 0x1e, 0xdf, 0x34, 0xd2, 0x06, 0x96, 0x5e,
                                       0x86, 0xb3, 0xe9, 0x4f, 0x53, 0x6e, 0x42, 0x46};
  uint32_t rk[32];
  uint8_t ct[16];
  struct sigaction action;
  struct sigaction old_action;
  int ok = 0;

  memset(&action, 0, sizeof(action));
  action.sa_handler = hygon_cis_sigill_handler;
  sigemptyset(&action.sa_mask);

  if (sigaction(SIGILL, &action, &old_action) != 0) {
    return 0;
  }

  hygon_cis_sigill_active = 1;
  if (sigsetjmp(hygon_cis_sigill_jmp, 1) == 0) {
    hygon_cis_sm4_set_key(key, rk);
    hygon_cis_sm4_encrypt_block(rk, pt, ct);
    ok = memcmp(ct, expected, sizeof(expected)) == 0;
  } else {
    ok = 0;
  }
  hygon_cis_sigill_active = 0;

  sigaction(SIGILL, &old_action, NULL);
  return ok;
}

int hygon_cis_sm4_selftest(void) {
  return hygon_cis_sm4_run_selftest();
}

int hygon_cis_sm4_is_supported(void) {
  static int cached = -1;

  if (cached < 0) {
    cached = hygon_cis_sm4_run_selftest() ? 1 : 0;
  }
  return cached;
}
