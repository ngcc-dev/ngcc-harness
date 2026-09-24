#include <stdio.h>
#include "pspm.h"
#include "polyvec.h"
#include "params.h"
#include "neon_compat.h"
#include <string.h>

void poly_emulate_cs1(poly *r, const poly *c, const uint8_t s1_table[PARAM_N * 3])
{
    ALIGN(32)
    uint8_t w[PARAM_N];
    uint8_t *s;
    __m256i w256, s256, f0, f1, f2, f3, t;

    memset(w, 0, 512);
    for (int i = 0; i < PARAM_N; i++)
    {
        if (c->coeffs[i] != 0)
        {
            s = s1_table + PARAM_N - i + (PARAM_N & (c->coeffs[i] >> 31));
            for (int j = 0; j < PARAM_N / 32; j++)
            {
                w256 = _mm256_loadu_si256(w + j * 32);
                s256 = _mm256_loadu_si256(s + j * 32);
                w256 = _mm256_add_epi8(w256, s256);
                _mm256_storeu_si256(w + j * 32, w256);
            }
        }
    }
    for (int i = 0; i < PARAM_N / 32; i++)
    {
        w256 = _mm256_loadu_si256(w + i * 32);
        t = _mm256_bsrli_epi128(w256, 8);

        f0 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(w256));
        f1 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(t));
        f2 = _mm256_cvtepi8_epi32(_mm256_extractf128_si256(w256, 1));
        f3 = _mm256_cvtepi8_epi32(_mm256_extractf128_si256(t, 1));

        _mm256_storeu_si256(r->coeffs + 32 * i, f0);
        _mm256_storeu_si256(r->coeffs + 32 * i + 8, f1);
        _mm256_storeu_si256(r->coeffs + 32 * i + 16, f2);
        _mm256_storeu_si256(r->coeffs + 32 * i + 24, f3);
    }
}


inline uint32_t check_norm(__m256i b, int bound) {
    const __m256i boundx8 = _mm256_set1_epi32(bound);
	const __m256i onex8 = _mm256_set1_epi32(1);
    __m256i  c , r;

    c = _mm256_srai_epi32(b, 31);
    c = _mm256_and_si256(c, onex8);
    b = _mm256_abs_epi32(b);
    b = _mm256_add_epi32(b,c);
    b = _mm256_cmpgt_epi32(b, boundx8);
    return _mm256_movemask_epi8(b); 
    
}


int poly_emulate_z(poly *r, const poly *y, const poly *c, const uint8_t s1_table[PARAM_N * 3])
{
    ALIGN(32)
    uint8_t w[PARAM_N];

    uint8_t *s;
    __m256i w256, s256, f0, f1, f2, f3, t;
    __m256i g0,g1,g2,g3;
    uint32_t res = 0;

    memset(w, 0, 512);
    for (int i = 0; i < PARAM_N; i++)
    {
        if (c->coeffs[i] != 0)
        {
            s = s1_table + PARAM_N - i + (PARAM_N & (c->coeffs[i] >> 31));
            for (int j = 0; j < PARAM_N / 32; j++)
            {
                w256 = _mm256_loadu_si256(w + j * 32);
                s256 = _mm256_loadu_si256(s + j * 32);
                w256 = _mm256_add_epi8(w256, s256);
                _mm256_storeu_si256(w + j * 32, w256);
            }
        }
    }
    for (int i = 0; i < PARAM_N / 32; i++)
    {
        w256 = _mm256_loadu_si256(w + i * 32);
        t = _mm256_bsrli_epi128(w256, 8);

        f0 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(w256));
        f1 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(t));
        f2 = _mm256_cvtepi8_epi32(_mm256_extractf128_si256(w256, 1));
        f3 = _mm256_cvtepi8_epi32(_mm256_extractf128_si256(t, 1));

        g0 = _mm256_load_si256(y->coeffs + 32 * i);
        g1 = _mm256_load_si256(y->coeffs + 32 * i + 8);
        g2 = _mm256_load_si256(y->coeffs + 32 * i + 16);
        g3 = _mm256_load_si256(y->coeffs + 32 * i + 24);

        f0 = _mm256_add_epi32(f0, g0);
        f1 = _mm256_add_epi32(f1, g1);
        f2 = _mm256_add_epi32(f2, g2);
        f3 = _mm256_add_epi32(f3, g3); 

        if (check_norm(f0, GAMMA1 - BETA1)) return 1;
        if (check_norm(f1, GAMMA1 - BETA1)) return 1;
        if (check_norm(f2, GAMMA1 - BETA1)) return 1;
        if (check_norm(f3, GAMMA1 - BETA1)) return 1;

        _mm256_storeu_si256(r->coeffs + 32 * i, f0);
        _mm256_storeu_si256(r->coeffs + 32 * i + 8, f1);
        _mm256_storeu_si256(r->coeffs + 32 * i + 16, f2);
        _mm256_storeu_si256(r->coeffs + 32 * i + 24, f3);
    }

    return 0;
}

int emulate_ct(polyveck *r, const poly *c, const polyveck *t)
{
    ALIGN(32) int32_t stable[PARAM_N * 3];

    int32_t *stable32;
    __m256i w256, s256, f0, f1, f2, f3;
    const __m256i zero = _mm256_setzero_si256();

    for (int k = 0; k < PARAM_K; k++)
    {
        int32_t *w = r->vec[k].coeffs;
        memset(w, 0, PARAM_N * 4);
        for (int i = 0; i < PARAM_N / 8; i++)
        {
            f0 = _mm256_load_si256(t->vec[k].coeffs + i * 8);
            f1 = _mm256_sub_epi32(zero, f0);
            _mm256_store_si256(stable + i * 8, f1);
            _mm256_store_si256(stable + PARAM_N * 2 + i * 8, f1);
            _mm256_store_si256(stable + PARAM_N + i * 8, f0);
        }

        for (int i = 0; i < PARAM_N; i++)
        {
            if (c->coeffs[i] != 0)
            {
                stable32 = stable + PARAM_N - i + (PARAM_N & (c->coeffs[i] >> 31));
                for (int j = 0; j < PARAM_N / 8; j++)
                {
                    w256 = _mm256_load_si256(w + j * 8);
                    s256 = _mm256_loadu_si256(stable32 + j * 8);
                    w256 = _mm256_add_epi32(w256, s256);
                    _mm256_store_si256(w + j * 8, w256);
                }
            }
        }
    }
    return 0;
}

int poly_emulate_ct(poly *r, const poly *c, const poly *t)
{
    ALIGN(32)
    int32_t stable[PARAM_N * 3];

    int32_t *stable32;
    __m256i w256, s256, f0, f1, f2, f3;
    const __m256i zero = _mm256_setzero_si256();

    int32_t *w = r->coeffs;
    memset(w, 0, PARAM_N * 4);
    for (int i = 0; i < PARAM_N / 8; i++)
    {
        f0 = _mm256_load_si256(t->coeffs + i * 8);
        f1 = _mm256_sub_epi32(zero, f0);
        _mm256_store_si256(stable + i * 8, f1);
        _mm256_store_si256(stable + PARAM_N * 2 + i * 8, f1);
        _mm256_store_si256(stable + PARAM_N + i * 8, f0);
    }

    for (int i = 0; i < PARAM_N; i++)
    {
        if (c->coeffs[i] != 0)
        {
            stable32 = stable + PARAM_N - i + (PARAM_N & (c->coeffs[i] >> 31));
            for (int j = 0; j < PARAM_N / 8; j++)
            {
                w256 = _mm256_load_si256(w + j * 8);
                s256 = _mm256_loadu_si256(stable32 + j * 8);
                w256 = _mm256_add_epi32(w256, s256);
                _mm256_store_si256(w + j * 8, w256);
            }
        }
    }

    return 0;
}

#if PARAMS == 1

void emulate_cs2(polyveck *r, const poly *c, const uint16_t s2_table[PARAM_K][PARAM_N * 3])
{
    ALIGN(32) uint16_t w[PARAM_N];
    uint16_t *s;
    __m256i w256, s256, f0, f1, f2, f3, t;

    for (int k = 0; k < PARAM_K; k++)
    {
        memset(w, 0, PARAM_N * 2);
        for (int i = 0; i < PARAM_N; i++)
        {
            if (c->coeffs[i] != 0)
            {
                s = s2_table[k] + PARAM_N - i + (PARAM_N & (c->coeffs[i] >> 31));
                for (int j = 0; j < PARAM_N / 16; j++)
                {
                    w256 = _mm256_loadu_si256(w + j * 16);
                    s256 = _mm256_loadu_si256(s + j * 16);
                    w256 = _mm256_add_epi16(w256, s256);
                    _mm256_storeu_si256(w + j * 16, w256);
                }
            }
        }
        for (int i = 0; i < PARAM_N / 16; i++)
        {
            w256 = _mm256_loadu_si256(w + i * 16);
            s256 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(w256,0));
            w256 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(w256,1));
            _mm256_storeu_si256(r->vec[k].coeffs + i * 16, s256);
            _mm256_storeu_si256(r->vec[k].coeffs + i * 16 + 8, w256);
        }
    }
}

void poly_emulate_cs2(poly *r, const poly *c, const uint16_t s2_table[PARAM_N * 3])
{
    ALIGN(32)
    uint16_t w[PARAM_N];
    uint16_t *s;
    __m256i w256, s256, f0, f1, f2, f3, t;

    memset(w, 0, PARAM_N * 2);
    for (int i = 0; i < PARAM_N; i++)
    {
        if (c->coeffs[i] != 0)
        {
            s = s2_table + PARAM_N - i + (PARAM_N & (c->coeffs[i] >> 31));
            for (int j = 0; j < PARAM_N / 16; j++)
            {
                w256 = _mm256_loadu_si256(w + j * 16);
                s256 = _mm256_loadu_si256(s + j * 16);
                w256 = _mm256_add_epi16(w256, s256);
                _mm256_storeu_si256(w + j * 16, w256);
            }
        }
    }
    for (int i = 0; i < PARAM_N / 16; i++)
    {
        w256 = _mm256_loadu_si256(w + i * 16);
        s256 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(w256, 0));
        w256 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(w256, 1));
        _mm256_storeu_si256(r->coeffs + i * 16, s256);
        _mm256_storeu_si256(r->coeffs + i * 16 + 8, w256);
    }
}

int poly_emulate_w0_cs2(poly *r, const poly *w0, const poly *c, const uint16_t s2_table[PARAM_N * 3])
{
    ALIGN(32)
    uint16_t w[PARAM_N];
    uint16_t *s;
    __m256i w256, s256, f0, f1, f2, f3, t;
    __m256i g0,g1;
    uint32_t res = 0;

    memset(w, 0, PARAM_N * 2);
    for (int i = 0; i < PARAM_N; i++)
    {
        if (c->coeffs[i] != 0)
        {
            s = s2_table + PARAM_N - i + (PARAM_N & (c->coeffs[i] >> 31));
            for (int j = 0; j < PARAM_N / 16; j++)
            {
                w256 = _mm256_loadu_si256(w + j * 16);
                s256 = _mm256_loadu_si256(s + j * 16);
                w256 = _mm256_add_epi16(w256, s256);
                _mm256_storeu_si256(w + j * 16, w256);
            }
        }
    }
    for (int i = 0; i < PARAM_N / 16; i++)
    {
        w256 = _mm256_load_si256(w + i * 16);
        s256 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(w256, 0));
        w256 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(w256, 1));
        
        g0 = _mm256_load_si256(w0->coeffs + i * 16);
        g1 = _mm256_load_si256(w0->coeffs + i * 16 + 8);

        s256 = _mm256_sub_epi32(g0, s256);
        w256 = _mm256_sub_epi32(g1, w256);

        if (check_norm(s256, GAMMA2 - BETA2 - ETA1)) return 1;
        if (check_norm(w256, GAMMA2 - BETA2 - ETA1)) return 1;

        _mm256_storeu_si256(r->coeffs + i * 16, s256);
        _mm256_storeu_si256(r->coeffs + i * 16 + 8, w256);
    }

    return 0;
}

#else 

void emulate_cs2(polyveck *r, const poly *c, const uint8_t s2_table[PARAM_K][PARAM_N * 3])
{

    ALIGN(32) uint8_t w[PARAM_N];
    uint8_t *s;
    __m256i w256, s256, f0, f1, f2, f3, t;

    for (int l = 0; l < PARAM_K; l++)
    {
        memset(w, 0, 512);
        for (int i = 0; i < PARAM_N; i++)
        {
            if (c->coeffs[i] != 0)
            {
                s = s2_table[l] + PARAM_N - i + (PARAM_N & (c->coeffs[i] >> 31));
                for (int j = 0; j < PARAM_N / 32; j++)
                {
                    w256 = _mm256_loadu_si256(w + j * 32);
                    s256 = _mm256_loadu_si256(s + j * 32);
                    w256 = _mm256_add_epi8(w256, s256);
                    _mm256_storeu_si256(w + j * 32, w256);
                }
            }
        }
        for (int i = 0; i < PARAM_N / 32; i++)
        {
            w256 = _mm256_loadu_si256(w + i * 32);
            t = _mm256_bsrli_epi128(w256, 8);

            f0 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(w256));
            f1 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(t));
            f2 = _mm256_cvtepi8_epi32(_mm256_extractf128_si256(w256, 1));
            f3 = _mm256_cvtepi8_epi32(_mm256_extractf128_si256(t, 1));

            _mm256_storeu_si256(r->vec[l].coeffs + 32 * i, f0);
            _mm256_storeu_si256(r->vec[l].coeffs + 32 * i + 8, f1);
            _mm256_storeu_si256(r->vec[l].coeffs + 32 * i + 16, f2);
            _mm256_storeu_si256(r->vec[l].coeffs + 32 * i + 24, f3);
        }
    }
}

void poly_emulate_cs2(poly *r, const poly *c, const uint8_t s2_table[PARAM_N * 3])
{

    ALIGN(32)
    uint8_t w[PARAM_N];
    uint8_t *s;
    __m256i w256, s256, f0, f1, f2, f3, t;

    memset(w, 0, 512);
    for (int i = 0; i < PARAM_N; i++)
    {
        if (c->coeffs[i] != 0)
        {
            s = s2_table + PARAM_N - i + (PARAM_N & (c->coeffs[i] >> 31));
            for (int j = 0; j < PARAM_N / 32; j++)
            {
                w256 = _mm256_loadu_si256(w + j * 32);
                s256 = _mm256_loadu_si256(s + j * 32);
                w256 = _mm256_add_epi8(w256, s256);
                _mm256_storeu_si256(w + j * 32, w256);
            }
        }
    }
    for (int i = 0; i < PARAM_N / 32; i++)
    {
        w256 = _mm256_loadu_si256(w + i * 32);
        t = _mm256_bsrli_epi128(w256, 8);

        f0 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(w256));
        f1 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(t));
        f2 = _mm256_cvtepi8_epi32(_mm256_extractf128_si256(w256, 1));
        f3 = _mm256_cvtepi8_epi32(_mm256_extractf128_si256(t, 1));

        _mm256_storeu_si256(r->coeffs + 32 * i, f0);
        _mm256_storeu_si256(r->coeffs + 32 * i + 8, f1);
        _mm256_storeu_si256(r->coeffs + 32 * i + 16, f2);
        _mm256_storeu_si256(r->coeffs + 32 * i + 24, f3);
    }
}

int poly_emulate_w0_cs2(poly *r, const poly *w0, const poly *c, const uint8_t s2_table[PARAM_N * 3])
{

    ALIGN(32)
    uint8_t w[PARAM_N];
    uint8_t *s;
    __m256i w256, s256, f0, f1, f2, f3, t;
    __m256i g0,g1,g2,g3;

    memset(w, 0, PARAM_N);
    for (int i = 0; i < PARAM_N; i++)
    {
        if (c->coeffs[i] != 0)
        {
            s = s2_table + PARAM_N - i + (PARAM_N & (c->coeffs[i] >> 31));
            for (int j = 0; j < PARAM_N / 32; j++)
            {
                w256 = _mm256_loadu_si256(w + j * 32);
                s256 = _mm256_loadu_si256(s + j * 32);
                w256 = _mm256_add_epi8(w256, s256);
                _mm256_storeu_si256(w + j * 32, w256);
            }
        }
    }
    for (int i = 0; i < PARAM_N / 32; i++)
    {
        w256 = _mm256_loadu_si256(w + i * 32);
        t = _mm256_bsrli_epi128(w256, 8);

        f0 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(w256));
        f1 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(t));
        f2 = _mm256_cvtepi8_epi32(_mm256_extractf128_si256(w256, 1));
        f3 = _mm256_cvtepi8_epi32(_mm256_extractf128_si256(t, 1));

        g0 = _mm256_load_si256(w0->coeffs + 32 * i);
        g1 = _mm256_load_si256(w0->coeffs + 32 * i + 8);
        g2 = _mm256_load_si256(w0->coeffs + 32 * i + 16);
        g3 = _mm256_load_si256(w0->coeffs + 32 * i + 24);

        f0 = _mm256_sub_epi32(g0,f0);
        f1 = _mm256_sub_epi32(g1,f1);
        f2 = _mm256_sub_epi32(g2,f2);
        f3 = _mm256_sub_epi32(g3,f3);

        if (check_norm(f0, GAMMA2 - BETA2 - ETA1)) return 1;
        if (check_norm(f1, GAMMA2 - BETA2 - ETA1)) return 1;
        if (check_norm(f2, GAMMA2 - BETA2 - ETA1)) return 1;
        if (check_norm(f3, GAMMA2 - BETA2 - ETA1)) return 1;

        _mm256_storeu_si256(r->coeffs + 32 * i, f0);
        _mm256_storeu_si256(r->coeffs + 32 * i + 8, f1);
        _mm256_storeu_si256(r->coeffs + 32 * i + 16, f2);
        _mm256_storeu_si256(r->coeffs + 32 * i + 24, f3);
    }
    return 0;
}

#endif