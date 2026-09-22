#include "ntt_2s3t.h"
#include <malloc.h>

// zeta = zeta_6
static inline void split(int16_t* f, int32_t len, int16_t zeta) {
    int32_t offset = len >> 1;

    int16_t X, Y, Z;
    int16_t* pX = f;
    int16_t* pY = f + offset;

    for (int32_t i = 0; i < offset; i++) {
        X = *pX;
        Y = *pY;

        Z = montgomery_reduce((int32_t)zeta * Y);
        *pX = X + Z;
        *pY = X + Y - Z;

        pX++;
        pY++;
    }
}

// zeta = zeta_6^5
static inline void compose(int16_t* f, int32_t len, int16_t zeta) {
    int32_t offset = len >> 1;

    int16_t X, Y, Z;
    int16_t* pX = f;
    int16_t* pY = f + offset;

    for (int32_t i = 0; i < offset; i++) {
        X = *pX;
        Y = *pY;

        Z = X - Y;
        X = X + Y;  // 2X+Y

        Z = montgomery_reduce((int32_t)zeta * Z);
        Z += Y;  // X+2Y

        Y = 2 * X - Z;  // 3X
        *pX = Y;
        *pY = X + Z - Y;  // 3Y

        pX++;
        pY++;
    }
}

// zeta^{j}
static inline void radix2(int16_t* f, int32_t len, int16_t zeta) {
    int32_t offset = len >> 1;

    int16_t X, Y, Z;
    int16_t* pX = f;
    int16_t* pY = f + offset;

    for (int32_t i = 0; i < offset; i++) {
        X = *pX;
        Y = *pY;

        Z = montgomery_reduce((int32_t)zeta * Y);
        *pX = X + Z;
        *pY = X - Z;

        pX++;
        pY++;
    }
}

// zeta^{-j}
static inline void radix2inv(int16_t* f, int32_t len, int16_t zeta) {
    int32_t offset = len >> 1;

    int16_t X, Y, Z;
    int16_t* pX = f;
    int16_t* pY = f + offset;

    for (int32_t i = 0; i < offset; i++) {
        X = *pX;
        Y = *pY;
        Z = X - Y;

        *pX = barrett_reduce(X + Y);                 // 2X
        *pY = montgomery_reduce((int32_t)zeta * Z);  // 2Y

        pX++;
        pY++;
    }
}

// zeta1, zeta2, zeta3, zeta1^2, zeta2^2, zeta3^2
static inline void radix3(int16_t* f, int32_t len, int16_t z1, int16_t z2, int16_t z3) {
    int32_t offset = len / 3;

    int16_t X, Y, Z;
    int16_t* pX = f;
    int16_t* pY = f + offset;
    int16_t* pZ = f + 2 * offset;

    int16_t w1 = montgomery_reduce((int32_t)z1 * z1);
    int16_t w2 = montgomery_reduce((int32_t)z2 * z2);
    int16_t w3 = montgomery_reduce((int32_t)z3 * z3);

    for (int32_t i = 0; i < offset; i++) {
        X = *pX;
        Y = *pY;
        Z = *pZ;

        *pX = X + montgomery_reduce((int32_t)z1 * Y) + montgomery_reduce((int32_t)w1 * Z);
        *pY = X + montgomery_reduce((int32_t)z2 * Y) + montgomery_reduce((int32_t)w2 * Z);
        *pZ = X + montgomery_reduce((int32_t)z3 * Y) + montgomery_reduce((int32_t)w3 * Z);

        pX++;
        pY++;
        pZ++;
    }
}

// zeta1^{-1}, zeta2^{-1}, zeta3^{-1}, zeta1^{-2}, zeta2^{-2}, zeta3^{-2}
static inline void radix3inv(int16_t* f, int32_t len, int16_t z1, int16_t z2, int16_t z3) {
    int32_t offset = len / 3;

    int16_t X, Y, Z;
    int16_t* pX = f;
    int16_t* pY = f + offset;
    int16_t* pZ = f + 2 * offset;

    int16_t w1 = montgomery_reduce((int32_t)z1 * z1);
    int16_t w2 = montgomery_reduce((int32_t)z2 * z2);
    int16_t w3 = montgomery_reduce((int32_t)z3 * z3);

    for (int32_t i = 0; i < offset; i++) {
        X = *pX;
        Y = *pY;
        Z = *pZ;

        *pX = barrett_reduce(X + Y + Z);                                                                                     // 3X
        *pY = montgomery_reduce((int32_t)z1 * X) + montgomery_reduce((int32_t)z2 * Y) + montgomery_reduce((int32_t)z3 * Z);  // 3Y
        *pZ = montgomery_reduce((int32_t)w1 * X) + montgomery_reduce((int32_t)w2 * Y) + montgomery_reduce((int32_t)w3 * Z);  // 3Z

        pX++;
        pY++;
        pZ++;
    }
}

// m = 2^s * 3^t
// split + (t-1) * radix-3 + (s-1) * radix-2
void ForwardNTT(int16_t* f) {
    int32_t p = 2;
    split(f, N, forward_zetas[0]);

    int32_t blocknum = 1;
    int32_t blocksize = N >> 1;

    int16_t* f0 = f;
    int16_t* f1 = f + blocksize;

    for (int32_t i = 1; i < T; i++) {
        for (int32_t j = 0; j < blocknum; j++) {
            radix3(f0 + j * blocksize, blocksize, forward_zetas[p], forward_zetas[p + 1], forward_zetas[p + 2]);
            p += 3;
        }

        for (int32_t j = 0; j < blocknum; j++) {
            radix3(f1 + j * blocksize, blocksize, forward_zetas[p], forward_zetas[p + 1], forward_zetas[p + 2]);
            p += 3;
        }

        blocknum *= 3;
        blocksize /= 3;
    }

    for (int32_t i = 1; i < S; i++) {
        for (int32_t j = 0; j < blocknum; j++) {
            radix2(f0 + j * blocksize, blocksize, forward_zetas[p]);
            p += 2;
        }

        for (int32_t j = 0; j < blocknum; j++) {
            radix2(f1 + j * blocksize, blocksize, forward_zetas[p]);
            p += 2;
        }

        blocknum <<= 1;
        blocksize >>= 1;
    }
}

// m = 2^s * 3^t
// (s-1) * inv-radix-2 + (t-1) * inv-radix-3 + compose
void InverseNTT(int16_t* f) {
    int32_t p = 0;
    int32_t blocknum = N >> 1;
    int32_t blocksize = 1;

    int16_t* f0 = f;
    int16_t* f1 = f + (N >> 1);

    for (int32_t i = 1; i < S; i++) {
        blocknum >>= 1;
        blocksize <<= 1;

        for (int32_t j = blocknum - 1; j >= 0; j--) {
            radix2inv(f1 + j * blocksize, blocksize, inverse_zetas[p + 1]);
            p += 2;
        }

        for (int32_t j = blocknum - 1; j >= 0; j--) {
            radix2inv(f0 + j * blocksize, blocksize, inverse_zetas[p + 1]);
            p += 2;
        }
    }

    for (int32_t i = 1; i < T; i++) {
        blocknum /= 3;
        blocksize *= 3;

        for (int32_t j = blocknum - 1; j >= 0; j--) {
            radix3inv(f1 + j * blocksize, blocksize, inverse_zetas[p + 2], inverse_zetas[p + 1], inverse_zetas[p]);
            p += 3;
        }

        for (int32_t j = blocknum - 1; j >= 0; j--) {
            radix3inv(f0 + j * blocksize, blocksize, inverse_zetas[p + 2], inverse_zetas[p + 1], inverse_zetas[p]);
            p += 3;
        }
    }

    compose(f, N, inverse_zetas[p + 1]);

    int16_t F1, F2, F3, F4;
    for (int32_t i = 0; i < N; i += 4) {
        F1 = montgomery_reduce((int32_t)f[i] * InvFactor);
        f[i] = F1 + ((F1 >> 15) & Q);

        F2 = montgomery_reduce((int32_t)f[i + 1] * InvFactor);
        f[i + 1] = F2 + ((F2 >> 15) & Q);

        F3 = montgomery_reduce((int32_t)f[i + 2] * InvFactor);
        f[i + 2] = F3 + ((F3 >> 15) & Q);

        F4 = montgomery_reduce((int32_t)f[i + 3] * InvFactor);
        f[i + 3] = F4 + ((F4 >> 15) & Q);
    }
}

void NTTMul(int16_t* r, const int16_t* a, const int16_t* b) {
    for (int32_t i = 0; i < N; i += 4) {
        r[i] = (int32_t)a[i] * b[i];
        r[i + 1] = (int32_t)a[i + 1] * b[i + 1];
        r[i + 2] = (int32_t)a[i + 2] * b[i + 2];
        r[i + 3] = (int32_t)a[i + 3] * b[i + 3];
    }
}

void NTTAdd(int16_t* r, const int16_t* a, const int16_t* b) {
    for (int32_t i = 0; i < N; i += 4) {
        r[i] = a[i] + b[i];
        r[i + 1] = a[i + 1] + b[i + 1];
        r[i + 2] = a[i + 2] + b[i + 2];
        r[i + 3] = a[i + 3] + b[i + 3];
    }
}

void NTTMod(int16_t* r, const int16_t* a) {
    int16_t x0, x1, x2, x3;
    for (int32_t i = 0; i < N; i += 4) {
        r[i] = barrett_reduce(a[i]);
        r[i + 1] = barrett_reduce(a[i + 1]);
        r[i + 2] = barrett_reduce(a[i + 2]);
        r[i + 3] = barrett_reduce(a[i + 3]);
    }
}