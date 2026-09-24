#ifndef PARAMS_H
#define PARAMS_H

#include "config.h"


/*-----------------128 bits security parameters-----------------*/
#if KEM_MODE == 128

#define N 512
#define Q 769
#define Q_BITS 10
#define HALF_Q 385  // (Q+1) / 2
#define DQ (Q << 1) // 2 * Q

#define D 8 // 压缩后的比特数

#define PF  125
#define PG  3125
#define PR  125
#define PE  3125

#define SEEDBYTES 32

#define KEM_MSGBYTES SEEDBYTES
#define KEM_CPAPKE_MSGBYTES SEEDBYTES
#define KEM_POLYBYTES 615
#define KEM_POLYCOMPRESSEDBYTES (N * D / 8)

#define F1_N 256
#define REPETITIONS (N / F1_N)
#define F1_N_WORDS (F1_N / 64)

// CPA_PKE size parameters
#define KEM_CPAPKE_PUBLICKEYBYTES KEM_POLYBYTES
#define KEM_CPAPKE_SECRETKEYBYTES (N * Q_BITS / 8 + SEEDBYTES)
#define KEM_CPAPKE_CIPHERTEXTBYTES KEM_POLYCOMPRESSEDBYTES

// KEM size parameters
#define KEM_PUBLICKEYBYTES KEM_POLYBYTES
#define KEM_SECRETKEYBYTES (KEM_CPAPKE_SECRETKEYBYTES + KEM_CPAPKE_PUBLICKEYBYTES + 2 * SEEDBYTES)
#define KEM_CIPHERTEXTBYTES KEM_CPAPKE_CIPHERTEXTBYTES


/*-----------------256 bits security parameters-----------------*/
#elif KEM_MODE == 256

#define N 1024
#define Q 769
#define Q_BITS 10
#define HALF_Q 385  // (Q+1) / 2
#define DQ (Q << 1) // 2 * Q

#define D 8 // 压缩后的比特数

#define PF  15625
#define PG  25
#define PR  15625
#define PE  25

#define SEEDBYTES 32

#define KEM_MSGBYTES SEEDBYTES
#define KEM_CPAPKE_MSGBYTES SEEDBYTES
#define KEM_POLYBYTES 1229
#define KEM_POLYCOMPRESSEDBYTES (N * D / 8)

#define F1_N 256
#define REPETITIONS (N / F1_N)
#define F1_N_WORDS (F1_N / 64)

// CPA_PKE size parameters
#define KEM_CPAPKE_PUBLICKEYBYTES KEM_POLYBYTES
#define KEM_CPAPKE_SECRETKEYBYTES (N * Q_BITS / 8 + SEEDBYTES)
#define KEM_CPAPKE_CIPHERTEXTBYTES KEM_POLYCOMPRESSEDBYTES

// KEM size parameters
#define KEM_PUBLICKEYBYTES KEM_POLYBYTES
#define KEM_SECRETKEYBYTES (KEM_CPAPKE_SECRETKEYBYTES + KEM_CPAPKE_PUBLICKEYBYTES + 2 * SEEDBYTES)
#define KEM_CIPHERTEXTBYTES KEM_CPAPKE_CIPHERTEXTBYTES


/*-----------------512 bits security parameters-----------------*/
#elif KEM_MODE == 512

#define N 2048
#define Q 3329
#define Q_BITS 12
#define HALF_Q 1665  // (Q+1) / 2
#define DQ (Q << 1) // 2 * Q

#define D 9 // 压缩后的比特数

#define PF  4375
#define PG  4375
#define PR  4375
#define PE  4375

#define SEEDBYTES 64

#define KEM_MSGBYTES SEEDBYTES
#define KEM_CPAPKE_MSGBYTES SEEDBYTES
#define KEM_POLYBYTES 3072
#define KEM_POLYCOMPRESSEDBYTES (N * D / 8)

#define F1_N 512
#define REPETITIONS (N / F1_N)
#define F1_N_WORDS (F1_N / 64)

// CPA_PKE size parameters
#define KEM_CPAPKE_PUBLICKEYBYTES KEM_POLYBYTES
#define KEM_CPAPKE_SECRETKEYBYTES (N * Q_BITS / 8 + SEEDBYTES)
#define KEM_CPAPKE_CIPHERTEXTBYTES KEM_POLYCOMPRESSEDBYTES

// KEM size parameters
#define KEM_PUBLICKEYBYTES KEM_POLYBYTES
#define KEM_SECRETKEYBYTES (KEM_CPAPKE_SECRETKEYBYTES + KEM_CPAPKE_PUBLICKEYBYTES + 2 * SEEDBYTES)
#define KEM_CIPHERTEXTBYTES KEM_CPAPKE_CIPHERTEXTBYTES

#endif 

#endif