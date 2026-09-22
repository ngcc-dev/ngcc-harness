/*
 * Scaled-down stand-in for the CS-128 params.h, used ONLY by the
 * reproduce_forgery_scaled driver.  No submission file is modified; this file
 * is placed earlier on the include path so the CS-128 sources compile against
 * it instead.
 *
 * The forgery of reproduce_forgery.c costs C(n,tau) hash calls.  At the
 * submitted tau = 23 that is 2^108.08, which is a real but not runnable
 * number, so this instance lowers tau to 3 (C(256,3) = 2^21.4) and leaves
 * everything else structurally identical.  The three companion edits exist
 * only to keep the rANS alphabets that encodings.c hard-codes the same size,
 * and they keep all three verification bounds bit-identical to CS-128:
 *
 *   tau  23 -> 3, taup 4 (so N = ceil(tau/taup) = 1, was 6)
 *   B0  116 -> 111   keeps M_HB_Z0 = 2*((B0-N)+1)+1 = 223 and B0-N   = 110
 *   B1 2779 -> 2759  keeps M_HB_Z1 = 2*(((B1-tau)>>d1)+1)+1 = 89 and B1-tau = 2756
 *   B2 1962 -> 1971  keeps M_HB_Z2 = 2*((2*(B2-tau)+tau*2^beta)/alpha+1)+1 = 7
 *
 * B2' = B2 - tau + alpha/4 + 1 + tau*2^(beta-1) becomes 2521 (was 2812).
 * M0 keeps only its first entry because N = 1; the rejection constant for a
 * single iteration with taup > tau is not the tuned one, so this instance is
 * used for verification only and its signing rate is not meaningful.
 */
#ifndef PARAMS_H
#define PARAMS_H

#define lambda 128

//Do not change anything under this line.

#if lambda == 128
#define ALGORITHM_NAME "CS_128"
#define q 32257
#define n 256
#define k 3
#define l 3
#define eta 1
#define tau 3
#define taup 4
#define alpha 2016
#define beta 5
#define a0 5
#define b0 5
#define B0 111
#define a1 10
#define b1 9
#define B1 2759
#define a2 6
#define b2 9
#define B2 1971
#define d0 0
#define d1 6
#define bq 15 // bit length of q
static unsigned long long M0[] = { 16754060490623551488 };
#elif lambda == 256
#define ALGORITHM_NAME "CS_256"
#define q 64513
#define n 512
#define k 3
#define l 3
#define eta 1
#define tau 44
#define taup 3
#define alpha 8064
#define beta 7
#define a0 6
#define b0 6
#define B0 278
#define a1 9
#define b1 10
#define B1 5385
#define a2 9
#define b2 9
#define B2 2569
#define d0 2
#define d1 6
#define bq 16 // bit length of q
static unsigned long long M0[] = { 17620290888719722496, 17621784278981462016, 17623226973804584960, 17624617865583456256, 17625955695161976832, 17627239040357314560, 17628466310185975808, 17629635734405001216, 17630745355846195200, 17631793025200351232, 17632776383313438720, 17633692857687846912, 17634539649000501248, 17635313719477792768, 18071621945441501184 };
#elif lambda == 512
#define ALGORITHM_NAME "CS_512"
#define q 64513
#define n 512
#define k 6
#define l 5
#define eta 1
#define tau 118
#define taup 6
#define alpha 4032
#define beta 5
#define a0 6
#define b0 6
#define B0 261
#define a1 12
#define b1 11
#define B1 12793
#define a2 8
#define b2 11
#define B2 9771
#define d0 2
#define d1 7
#define bq 16 // bit length of q
static unsigned long long M0[] = { 17819932119395569664, 17820095570143191040, 17820168299748177920, 17820145520275107840, 17820022064670330880, 17819792397902534656, 17819450603968778240, 17818990350020651008, 17818404877047054336, 17817686985319770112, 17816829001564188672, 17815822777217662976, 17814659644361285632, 17813330417851742208, 17811825350397577216, 17810134137097097216, 17808245855887214592, 17806148981017155584, 17803831322687092736, 18120912975124414464 };
#else
#error "lambda must be in {128, 192, 256}"
#endif

#define dq (2 * q) // bit length of q
#define bound0  (a0 * ((1 << b0) - 1))
#define bound1  (a1 * ((1 << b1) - 1))

#define HBYTES (lambda / 4)
#define SEEDBYTES (lambda / 8)
#define POLYT0_PACKEDBYTES   (n * beta / 8)
#define POLYT1_PACKEDBYTES   (n * (bq - beta) / 8)
#define POLYZ0L_PACKEDBYTES  (n * d0 / 8)
#define POLYZ1L_PACKEDBYTES  (n * d1 / 8)
#if alpha == 2016
#define POLYW_PACKEDBYTES  (n * (bq - 10) / 8)
#elif alpha == 4032
#define POLYW_PACKEDBYTES  (n * (bq - 11) / 8)
#elif alpha == 8064
#define POLYW_PACKEDBYTES  (n * (bq - 12) / 8)
#else
#error "alpha must be in {2016, 4032, 8064}"
#endif
#define POLYC_PACKEDBYTES  HBYTES
#define POLYETA_PACKEDBYTES  (n * 2 / 8)

#if lambda == 128
#define POLYZ0H_PACKEDBYTES		240
#define POLYZ1H_PACKEDBYTES		545
#define POLYZ2_PACKEDBYTES		155
#elif lambda == 256
#define POLYZ0H_PACKEDBYTES		405
#define POLYZ1H_PACKEDBYTES		1250
#define POLYZ2_PACKEDBYTES		165
#elif lambda == 512
#define POLYZ0H_PACKEDBYTES		404
#define POLYZ1H_PACKEDBYTES		2135
#define POLYZ2_PACKEDBYTES		940
#endif

#define PUBLICKEYBYTES (SEEDBYTES + k * POLYT1_PACKEDBYTES)
#define SECRETKEYBYTES (2 * SEEDBYTES + HBYTES + l * POLYETA_PACKEDBYTES + k * (POLYETA_PACKEDBYTES + POLYT0_PACKEDBYTES + POLYT1_PACKEDBYTES))
#define SIGNATUREBYTES (POLYC_PACKEDBYTES + POLYZ0H_PACKEDBYTES + POLYZ0L_PACKEDBYTES + POLYZ1H_PACKEDBYTES + l * POLYZ1L_PACKEDBYTES + POLYZ2_PACKEDBYTES)

#endif
