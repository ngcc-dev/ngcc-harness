#ifndef PARAMS_H
#define PARAMS_H

#define LAMBDA 160

#define N 4096
#define MU 12

#define K 276
#define FE_BYTES ((K+7)/8)

#define PK_SEEDLEN (LAMBDA/8)
#define PK_SIZE (PK_SEEDLEN + FE_BYTES)

#define SK_SEEDLEN (LAMBDA/8)
#define SK_SIZE (SK_SEEDLEN + PK_SEEDLEN)


#define SALT_SIZE (LAMBDA*2/8)
#define RSEED_SIZE (LAMBDA/8)

#define COMMIT_SIZE (LAMBDA*2/8)
#define NODE_SIZE (LAMBDA/8)

#define N_TUPLE 140
#define N_TUPLE_SIZE ((N_TUPLE-2 + 7) / 8)

#define Omega 6
#define TAU 12
#define T_OPEN 125
#define N_LEAVES (N * TAU)


#define SIG_SIZE (SALT_SIZE + sizeof(long long) + (COMMIT_SIZE) + NODE_SIZE * T_OPEN + (COMMIT_SIZE) * TAU + (K * TAU+(N_TUPLE-2)*TAU*2+7)/8)

#endif