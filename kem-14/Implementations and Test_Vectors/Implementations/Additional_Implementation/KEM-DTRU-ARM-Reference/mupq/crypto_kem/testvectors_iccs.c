// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
//
// ICCS-format KAT generator for DTRU, designed to run on the target
// (physical board or QEMU) and stream the test vectors over the HAL UART.
//
// The deterministic SM3-DRNG (drng.c) seeds both the per-record seed stream
// and the per-record KEM randomness, so the output is fully reproducible and
// matches the layout produced by the ICCS reference KAT_KEM.c on the host:
//
//     ====================        <- start marker, consumed by the host runner
//     Count = 0
//     Seed_Len = 64
//     Seed = ...
//     PK_Len = ...
//     PK = ...
//     SK_Len = ...
//     SK = ...
//     CT_Len = ...
//     CT = ...
//     SS_Len = ...
//     SS = ...
//     <blank line>
//     ... (10 records) ...
//     #                           <- end marker, consumed by the host runner
//
// The host-side kat.py captures everything between the markers and writes it
// to the scheme's Test_Vectors/KAT_KEM_*.txt file.

#include "dtru_kem.h"
#include "hal.h"

#include <stdio.h>
#include <string.h>

#define SEED_LEN_BYTES 64
#define NTESTS 10

// Single reusable line buffer. SK is the largest object we ever hex-encode.
static char line[2 * DTRU_KEM_SECRETKEYBYTES + 64];

static void send_hex(const char *id, const unsigned char *msg, unsigned long long len)
{
  int n = sprintf(line, "%s", id);
  for (unsigned long long i = 0; i < len; i++)
    n += sprintf(line + n, "%02X", msg[i]);
  hal_send_str(line);
}

static void send_uint(const char *id, unsigned long long value)
{
  sprintf(line, "%s%u", id, (unsigned int)value);
  hal_send_str(line);
}

int main(void)
{
  unsigned char nonce[SEED_LEN_BYTES];
  unsigned char seed[SEED_LEN_BYTES];
  DRNG_ctx drng_seed;

  static unsigned char pk[DTRU_KEM_PUBLICKEYBYTES];
  static unsigned char sk[DTRU_KEM_SECRETKEYBYTES];
  static unsigned char ct[DTRU_KEM_CIPHERTEXTBYTES];
  static unsigned char ss[DTRU_SHAREDKEYBYTES];
  static unsigned char ss1[DTRU_SHAREDKEYBYTES];
  unsigned long long pk_len, sk_len, ss_len, ct_len;

  hal_setup(CLOCK_FAST);

  // Start marker: at least four '=' so the host runner can locate the body.
  hal_send_str("==========================");

  // Seed stream is itself driven by a DRNG instantiated from the fixed nonce
  // "seedseed..." (64 bytes), exactly like the ICCS reference harness.
  for (int i = 0; i < SEED_LEN_BYTES / 4; i++)
    memcpy(nonce + 4 * i, "seed", 4);
  init_random_number(&drng_seed, nonce, SEED_LEN_BYTES);

  for (int i = 0; i < NTESTS; i++)
  {
    send_uint("Count = ", (unsigned long long)i);

    get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8);
    send_uint("Seed_Len = ", SEED_LEN_BYTES);
    send_hex("Seed = ", seed, SEED_LEN_BYTES);

    // (Re)seed the KEM randomness for this record.
    dtru_init_drng(seed);

    if (kem_keygen(pk, &pk_len, sk, &sk_len))
    {
      hal_send_str("ERROR: kem_keygen");
      hal_send_str("#");
      return -1;
    }
    send_uint("PK_Len = ", pk_len);
    send_hex("PK = ", pk, pk_len);
    send_uint("SK_Len = ", sk_len);
    send_hex("SK = ", sk, sk_len);

    if (kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len))
    {
      hal_send_str("ERROR: kem_enc");
      hal_send_str("#");
      return -1;
    }
    send_uint("CT_Len = ", ct_len);
    send_hex("CT = ", ct, ct_len);
    send_uint("SS_Len = ", ss_len);
    send_hex("SS = ", ss, ss_len);

    if (kem_dec(sk, sk_len, ct, ct_len, ss1, &ss_len))
    {
      hal_send_str("ERROR: kem_dec");
      hal_send_str("#");
      return -1;
    }
    if (memcmp(ss, ss1, ss_len) != 0)
    {
      hal_send_str("ERROR: shared secret mismatch");
      hal_send_str("#");
      return -1;
    }

    // Blank line separates records (matches the reference layout).
    hal_send_str("");
  }

  // End marker.
  hal_send_str("#");
  return 0;
}
