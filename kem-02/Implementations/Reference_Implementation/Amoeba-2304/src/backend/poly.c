#include "poly.h"
#include <malloc.h>
#include <string.h>

// Z_{Q}[X]/(X^{KN} - X^{KN/2} + 1) = A[X]/(X^K - Y)
// A = Z_{Q}[Y]/(X^{KN} - X^{KN/2} + 1)
void ForwardNussbamuer(int16_t* F, const int16_t* f) {
    size_t size = sizeof(int16_t) * K * N;
    int16_t* TMP = (int16_t*)malloc(size);

    if (TMP) {
        for (int32_t i = 0; i < K; i++) {
            for (int32_t j = 0; j < N; j++) {
                TMP[i * N + j] = f[i + j * K];
            }
            ForwardNTT(TMP + i * N);
        }

        memcpy(F, TMP, size);
    }
    free(TMP);
}

void InverseNussbamuer(int16_t* f, const int16_t* F) {
    size_t size = sizeof(int16_t) * K * N;
    int16_t* TMP = (int16_t*)malloc(size);

    if (TMP) {
        memcpy(TMP, F, size);

        for (int32_t i = 0; i < K; i++) {
            InverseNTT(TMP + i * N);
            for (int32_t j = 0; j < N; j++) {
                f[i + j * K] = TMP[i * N + j];
            }
        }
    }
    free(TMP);
}

void PolyMul(int16_t* r, const int16_t* f, const int16_t* g) {
    size_t size = sizeof(int16_t) * K * N;
    int16_t* TMP = (int16_t*)malloc(size);

    if (TMP) {
        for (int32_t n = 0; n < N; n++) {
            int64_t Y = NTTofY[n];

            for (int32_t j = 0; j < K; j++) {
                int32_t i = 1;
                int64_t tmp = (int64_t)f[j * N + n] * g[n];

                for (; i <= j; i++) {
                    tmp += (int64_t)f[(j - i) * N + n] * g[i * N + n];  // f[j-i] * g[i] * X^{j}
                }
                for (; i < K; i++) {
                    tmp += (int64_t)f[(K + j - i) * N + n] * g[i * N + n] * Y;  // f[K+j-i] * g[i] * X^{K+j}, Y = X^K
                }

                TMP[j * N + n] = tmp % Q;
            }
        }
        memcpy(r, TMP, K * N * sizeof(int16_t));
    }
    free(TMP);
}

void PolyAdd(int16_t* r, const int16_t* f, const int16_t* g) {
    for (int32_t i = 0; i < K * N; i += 4) {
        r[i] = f[i] + g[i];
        r[i + 1] = f[i + 1] + g[i + 1];
        r[i + 2] = f[i + 2] + g[i + 2];
        r[i + 3] = f[i + 3] + g[i + 3];
    }
}

void PolySub(int16_t* r, const int16_t* f, const int16_t* g) {
    for (int32_t i = 0; i < K * N; i += 4) {
        r[i] = f[i] - g[i];
        r[i + 1] = f[i + 1] - g[i + 1];
        r[i + 2] = f[i + 2] - g[i + 2];
        r[i + 3] = f[i + 3] - g[i + 3];
    }
}

void PolyMod(int16_t* r, const int16_t* f) {
    int16_t x0, x1, x2, x3;
    for (int32_t i = 0; i < K * N; i += 4) {
        x0 = f[i] % Q;
        r[i] = x0 + ((x0 >> 15) & Q);
        x1 = f[i + 1] % Q;
        r[i + 1] = x1 + ((x1 >> 15) & Q);
        x2 = f[i + 2] % Q;
        r[i + 2] = x2 + ((x2 >> 15) & Q);
        x3 = f[i + 3] % Q;
        r[i + 3] = x3 + ((x3 >> 15) & Q);
    }
}

void PolyClear(int16_t* f) {
    memset(f, 0, RLWE_K * RLWE_N * sizeof(int16_t));
}