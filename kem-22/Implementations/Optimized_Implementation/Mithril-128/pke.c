#include "pke.h"

int32_t rrlwr_pke_zetas[RRLWR_N] __attribute__((aligned(32))) = RRLWR_KEM_ZETAS;
#ifdef PRECOMPUTE_TWIST
int32_t precomputed_twist[RRLWR_N] __attribute__((aligned(32))) = {24818892, 11536462, 44334956, 16548189, 55725101, 49795213, 49275624, 11722981, 34425332, 47707762, 14909268, 42696035, 3519123, 9449011, 9968600, 47521243, 45709502, 16425748, 54479674, 52319362, 49971271, 6289318, 40036712, 6834890, 13534722, 42818476, 4764550, 6924862, 9272953, 52954906, 19207512, 52409334, 49392285, 35425675, 48273063, 12952780, 41199173, 5090838, 7710351, 51555458, 9851939, 23818549, 10971161, 46291444, 18045051, 54153386, 51533873, 7688766, 19576705, 45698794, 20115337, 38088316, 22956092, 19066540, 46889112, 45492720, 39667519, 13545430, 39128887, 21155908, 36288132, 40177684, 12355112, 13751504, 24078891, 7826385, 52970727, 26741741, 17246060, 49833341, 6843929, 9231004, 35165333, 51417839, 6273497, 32502483, 41998164, 9410883, 52400295, 50013220, 27070326, 52154296, 42527978, 54755881, 24560089, 9021829, 39142128, 9750767, 32173898, 7089928, 16716246, 4488343, 34684135, 50222395, 20102096, 49493457, 13216975, 33920602, 33374512, 54192349, 10604193, 7531552, 24599275, 6147429, 46027249, 25323622, 25869712, 5051875, 48640031, 51712672, 34644949, 53096795, 10686610, 25641518, 29216201, 31860748, 46226096, 41463114, 20606575, 23706125, 48557614, 33602706, 30028023, 27383476, 13018128, 17781110, 38637649, 35538099};
#endif

void poly_subp(poly *r, const poly *f, const poly *g) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    int32_t h2 = (int32_t)1 << (RRLWR_PKE_LOGP-(RRLWR_PKE_LOGT+1));      // p/(2*t)
    h2 -= (int32_t)1 << (RRLWR_PKE_LOGQ-(RRLWR_PKE_LOGP+1));             // p/(2*t) - q/(2*p)
    r->coeffs[i] = (f->coeffs[i] - g->coeffs[i] + h2) & (RRLWR_PKE_P-1); // Reduce mod p
  }
}

void poly_add_msg(poly *r, const unsigned char msg[RRLWR_PKE_MESSAGE_LEN]) {
  int32_t mj;

  for(unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
    for(unsigned int j = 0; j < RRLWR_N; j++) {
      mj = (msg[i*RRLWR_N/8 + (j >> 3)] >> (j & 0x7)) & 1;                 // Select coefficient j of polynomial i
      r[i].coeffs[j] += (int32_t)1 << (RRLWR_PKE_LOGQ-(RRLWR_PKE_LOGP+1)); // Add q/(2*p)
      r[i].coeffs[j] += -mj & ((int32_t)1 << (RRLWR_PKE_LOGP-1));          // Add p/2 if message bit is 1, otherwise not
      r[i].coeffs[j] &= (RRLWR_PKE_P-1);                                   // Reduce mod p
    }
  }
}

uint8_t ct_cmp(const unsigned char *c1, const unsigned char *c2) {
  uint8_t r = 0;
  for(unsigned int i = 0; i < RRLWR_KEM_CT_LEN; i++) {
    r |= (c1[i] ^ c2[i]); // Only non-zero if c1 != c2
  }

  return -((-(uint32_t)r) >> 31);
}

int pke_keygen(unsigned char pk[RRLWR_PKE_PK_LEN], unsigned char sk[RRLWR_PKE_SK_LEN],
               const unsigned char seedA[RRLWR_PKE_SEED_A_LEN], const unsigned char seedS[RRLWR_SEED_S_LEN]) {

  ring_element s __attribute__((aligned(32)));
  ring_element b __attribute__((aligned(32)));
  ring_element_Awin a __attribute__((aligned(32)));

  // Generate a with coefficients in [-q/2+1, q/2]
  ring_uniform_Awin(&a, RRLWR_PKE_LOGQ, seedA, RRLWR_PKE_SEED_A_LEN,
                    RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                    RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME,
                    rrlwr_pke_zetas);

  // Generate s with coefficients in [-2, 1] and pack
  ring_uniform(&s, RRLWR_PKE_LOG_ETA+1, seedS, RRLWR_SEED_S_LEN);
  ring_pack(sk, &s, RRLWR_PKE_LOG_ETA+1);

  // Compute b = round(p/q*A*s)
  ring_mul_Awin_round_xtoy_32(b.x, &a, &s, RRLWR_K, RRLWR_PKE_PRIME,
                              RRLWR_PKE_PRIMEINV, RRLWR_NTTINV_FINALCONST,
                              RRLWR_PKE_LOGQ, RRLWR_PKE_LOGP,
                              rrlwr_pke_zetas);

  // Pack the public key
  for (unsigned int i = 0; i < RRLWR_PKE_SEED_A_LEN; i++) {
    pk[i] = seedA[i];
  }
  ring_pack(pk + RRLWR_PKE_SEED_A_LEN, &b, RRLWR_PKE_LOGP);

  return 0;
}

int pke_encrypt(unsigned char ct[RRLWR_PKE_CT_LEN], const unsigned char pk[RRLWR_PKE_PK_LEN],
                const unsigned char m[RRLWR_PKE_MESSAGE_LEN], const unsigned char seedSp[RRLWR_SEED_S_LEN]) {

  ring_element sp __attribute__((aligned(32)));
  ring_element bp __attribute__((aligned(32)));
  ring_element_Awin a __attribute__((aligned(32)));
  ring_element_Awin b __attribute__((aligned(32)));
  poly vp[RRLWR_PKE_ELL];
  const unsigned char *seedA = &pk[0];

  // Generate a with coefficients in [-q/2+1, q/2]
  ring_uniform_Awin(&a, RRLWR_PKE_LOGQ, seedA, RRLWR_PKE_SEED_A_LEN,
                    RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                    RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME,
                    rrlwr_pke_zetas);

  // Generate s_prime with coefficients in [-2, 1]
  ring_uniform(&sp, RRLWR_PKE_LOG_ETA+1, seedSp, RRLWR_SEED_S_LEN);

  // Compute b_prime = round(p/q*A*s_prime)
  ring_mul_Awin_round_xtoy_32(bp.x, &a, &sp, RRLWR_K, RRLWR_PKE_PRIME,
                              RRLWR_PKE_PRIMEINV, RRLWR_NTTINV_FINALCONST,
                              RRLWR_PKE_LOGQ, RRLWR_PKE_LOGP,
                              rrlwr_pke_zetas);

  // Compute v_prime = b*s_prime
  ring_unpack_Awin_ncoeffs(&b, pk + RRLWR_PKE_SEED_A_LEN, RRLWR_PKE_LOGP,
                           RRLWR_PKE_ELL, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                           RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME,
                           rrlwr_pke_zetas);
  ring_mul_Awin_invntt_reduce_pow2_32(vp, &b, &sp, RRLWR_PKE_ELL,
                                      RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                                      RRLWR_NTTINV_FINALCONST,
                                      RRLWR_PKE_LOGP, rrlwr_pke_zetas); // Do not repeat NTT(s)

  // Convert message to polynomial representation and compute (v_prime + q/(2*p) + p/2*m) mod p
  poly_add_msg(vp, m);

  // Round from R_p to R_t with result in [-t/2+1, t/2] and pack into ciphertext buffer
  for(unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
    poly_compress(&vp[i], RRLWR_PKE_LOGP - RRLWR_PKE_LOGT); // Multiply by t/p and floor
    poly_pack(ct + i*RRLWR_PKE_PACKED_POLYT_LEN, &vp[i], RRLWR_PKE_LOGT);
  }

  // Return ct = (cm = vp, b) and sk = s
  ring_pack(ct + RRLWR_PKE_ELL*RRLWR_PKE_PACKED_POLYT_LEN, &bp, RRLWR_PKE_LOGP);

  return 0;
}

int pke_decrypt(unsigned char m[RRLWR_PKE_MESSAGE_LEN], 
                const unsigned char ct[RRLWR_PKE_CT_LEN], const unsigned char sk[RRLWR_PKE_SK_LEN]) {
  ring_element bp __attribute__((aligned(32)));
  ring_element s __attribute__((aligned(32)));
  ring_element_Awin bp_aw __attribute__((aligned(32)));
  poly v[RRLWR_PKE_ELL] __attribute__((aligned(32)));
  poly cm[RRLWR_PKE_ELL] __attribute__((aligned(32)));

  // Unpack s, b and cm and decompress cm
  ring_unpack(&s, sk, RRLWR_PKE_LOG_ETA+1);
  ring_unpack(&bp, ct + RRLWR_PKE_ELL*RRLWR_PKE_PACKED_POLYT_LEN, RRLWR_PKE_LOGP);
  for(unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
    poly_unpack(&cm[i], ct + i*RRLWR_PKE_PACKED_POLYT_LEN, RRLWR_PKE_LOGT);
    poly_decompress(&cm[i], RRLWR_PKE_LOGP - RRLWR_PKE_LOGT); // Multiply by p/t
  }

  // Compute v = b_prime*s
  ring_to_Awin_ncoeffs(&bp_aw, &bp, RRLWR_PKE_ELL,
                       RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                       RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME,
                       rrlwr_pke_zetas);
  ring_mul_Awin_reduce_pow2_32(v, &bp_aw, &s, RRLWR_PKE_ELL,
                               RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                               RRLWR_NTTINV_FINALCONST, RRLWR_PKE_LOGP,
                               rrlwr_pke_zetas);

  for(unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
    poly_subp(&v[i], &v[i], &cm[i]);                     // Compute (v - (p/t)*cm) mod p
    poly_round_xtoy(&v[i], &v[i], RRLWR_PKE_LOGP, 1);    // Round to a single-bit message polynomial m'
    poly_pack(m + i*RRLWR_PKE_PACKED_POLY1_LEN, &v[i], 1); // Convert the message polynomial to a bit string
  }

  return 0;
}
