#ifndef PARAMS_H
#define PARAMS_H

#if 1

#define MODE 3

//------------------------------------------------------------------------Global parameters------------------------------------------------------------------------------------------------------
#define SEEDBYTES 32U
#define CRHBYTES 48U

#define QBITS 23U
#define GAMMA1_BITS (QBITS - 4)
#define GAMMA2_BITS (GAMMA1_BITS - 1)

#define Q (1U << QBITS) //2^23
//#define ROOT_OF_UNITY 1753U //Not required
#define D 10U //PKDROP
#define SETABITS 6

//Added for poly_mult
#define N128 128
#define N64 64
#define N32 32
#define N16 16
#define N8 8
#define N_SB (N >> 2)
#define N_SB_RES (2*N_SB-1)

#define N8_SB_RES (2*N8-1)
#define N16_RES (2*N16-1)
#define N32_SB_RES (2*N32-1)

//-------------------------------------------------------------------------MODE dependent parameters-----------------------------------------------------------------------------------------------
#if MODE == 0
#define PBITS 18U
#define GAMMA1_BAR_BITS (GAMMA1_BITS + PBITS - QBITS)
#define GAMMA2_BAR_BITS (GAMMA1_BAR_BITS - 1)

#define P (1U << PBITS)
#define GAMMA1 (1U << GAMMA1_BITS)
#define GAMMA2 (GAMMA1 >> 1)
#define GAMMA1_BAR (1U << GAMMA1_BAR_BITS)
#define GAMMA2_BAR (GAMMA1_BAR >> 1)
#define ETA (1U << (QBITS - PBITS - 1))     //(Q/(2*P))

#define N 128U //256U
#define K 9U
#define L 8U
#define BETA1 1664U
#define BETA2 38U
#define KAPPA 31U
#define OMEGA 168U

#elif MODE == 1
#define PBITS 19U
#define GAMMA1_BAR_BITS (GAMMA1_BITS + PBITS - QBITS)
#define GAMMA2_BAR_BITS (GAMMA1_BAR_BITS - 1)

#define P (1U << PBITS)
#define GAMMA1 (1U << GAMMA1_BITS)
#define GAMMA2 (GAMMA1 >> 1)
#define GAMMA1_BAR (1U << GAMMA1_BAR_BITS)
#define GAMMA2_BAR (GAMMA1_BAR >> 1)
#define ETA (1U << (QBITS - PBITS - 1))     //(Q/(2*P))

#define N 128U //256U
#define K 13U
#define L 9U
#define BETA1 1638U
#define BETA2 77U
#define KAPPA 64U // KAPPA should be 69 not 64, due to limitation in the implementation, we are temperorily using 64
#define OMEGA 144U

#elif MODE == 2
#define PBITS 20U
#define GAMMA1_BAR_BITS (GAMMA1_BITS + PBITS - QBITS)
#define GAMMA2_BAR_BITS (GAMMA1_BAR_BITS - 1)

#define P (1U << PBITS)
#define GAMMA1 (1U << GAMMA1_BITS)
#define GAMMA2 (GAMMA1 >> 1)
#define GAMMA1_BAR (1U << GAMMA1_BAR_BITS)
#define GAMMA2_BAR (GAMMA1_BAR >> 1)
#define ETA (1U << (QBITS - PBITS - 1))     //(Q/(2*P))

#define N 256U //256U
#define K 8U
#define L 7U
#define BETA1 73U
#define BETA2 12U
#define KAPPA 60U
#define OMEGA 128U

#elif MODE == 3
#undef QBITS
#undef Q
#undef GAMMA1_BITS
#undef GAMMA2_BITS

#define QBITS 25U
#define GAMMA1_BITS (QBITS - 4)
#define GAMMA2_BITS (GAMMA1_BITS - 1)
#define PBITS 22U
#define GAMMA1_BAR_BITS (GAMMA1_BITS + PBITS - QBITS)
#define GAMMA2_BAR_BITS (GAMMA1_BAR_BITS - 1)

#define Q (1U << QBITS)
#define P (1U << PBITS)
#define GAMMA1 (1U << GAMMA1_BITS)
#define GAMMA2 (GAMMA1 >> 1)
#define GAMMA1_BAR (1U << GAMMA1_BAR_BITS)
#define GAMMA2_BAR (GAMMA1_BAR >> 1)
#define ETA (1U << (QBITS - PBITS - 1))     //(Q/(2*P))
//#define SETABITS 4U

#define N 512U //256U
#define K 8U
#define L 7U
#define BETA1 73U
#define BETA2 12U
#define KAPPA 60U
#define OMEGA 128U

#endif

#else

#define SEEDBYTES 32U
#define CRHBYTES 48U
#define N 256U
#define K 8U
#define L 7U
#define QBITS 23U
#define PBITS 21U
#define Q (1 << QBITS)
#define P (1 << PBITS)
#define GAMMA1 (Q / 16)
#define GAMMA2 (GAMMA1 / 2)
#define GAMMA1_BAR (1 << 17U)
#define GAMMA2_BAR (GAMMA1_BAR / 2)
#define BETA1 125U
#define BETA2 25U
#define KAPPA 60U
#define OMEGA 144U
#define ETA 8U
#define SETABITS 5U
#define D 12U

#endif

#define POL_P_SIZE_PACKED ((N*PBITS)/8)
#define POL_Q_SIZE_PACKED ((N*QBITS)/8)
#define POLT1_SIZE_PACKED ((N*(PBITS - D + 1))/8)
#define POLT0_SIZE_PACKED ((N*D)/8)

#define POLETA_SIZE_PACKED ((N*SETABITS)/8)
#define POLZ_SIZE_PACKED ((N*(QBITS - 3))/8)//Will change
#define POLW1_SIZE_PACKED ((N*4)/8)

#define POLVECK_SIZE_PACKED (K*POL_SIZE_PACKED)
#define POLVECL_SIZE_PACKED (L*POL_SIZE_PACKED)
#define PK_SIZE_PACKED (SEEDBYTES + K*POLT1_SIZE_PACKED)
#define SK_SIZE_PACKED (2*SEEDBYTES + L*POLETA_SIZE_PACKED + CRHBYTES + K*POLT0_SIZE_PACKED)
#define SIG_SIZE_PACKED (L*POLZ_SIZE_PACKED + ((9*(OMEGA>>3)) + K + 1) + (N/8 + 8)) //Have to check in code

#endif
