#ifndef PARAMS_H
#define PARAMS_H

#define lambda 128	/* Change this for different security strengths */

//Do not change anything under this line.

#if lambda == 128
#define ALGORITHM_NAME "CS_128"
#define q 32257
#define n 256
#define k 3
#define l 3
#define eta 1
#define tau 23
#define taup 4
#define alpha 2016
#define beta 5
#define a0 5
#define b0 5
#define B0 116
#define a1 10
#define b1 9
#define B1 2779
#define a2 6
#define b2 9
#define B2 1962
#define d0 0
#define d1 6
#define bq 15 // bit length of q
static unsigned long long M0[] = { 16754060490623551488, 16762958675265769472, 16770730982686846976, 16777320755843377152, 16782655996990345216, 17413884194018961408 };
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
