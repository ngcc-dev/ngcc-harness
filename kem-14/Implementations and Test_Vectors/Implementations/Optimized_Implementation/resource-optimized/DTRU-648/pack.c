#include "pack.h"
#include "params.h"

#if PK_PACK_OPT

#include "inverse.h"

uint16_t uint32_mod_uint14(uint32_t x, uint16_t m)
{
    uint32_t q;
    uint16_t r;
    uint32_divmod_uint14(&q, &r, x, m);
    return r;
}

void Encode(unsigned char *out, const unsigned char *out_end, const uint16_t *R, const uint16_t *M, const long long len)
{
    if (len == 1)
    {
        uint16_t r = R[0];
        uint16_t m = M[0];
        while (m > 1)
        {
            if (out >= out_end) return;
            *out++ = (unsigned char)r;
            r >>= 8;
            m = (m + 255) >> 8;
        }
    }

    if (len > 1)
    {
        uint16_t R2[(len + 1) / 2];
        uint16_t M2[(len + 1) / 2];
        long long i;

        for (i = 0; i < len - 1; i += 2)
        {
            uint32_t m0 = M[i];
            uint32_t r = R[i] + R[i + 1] * m0;
            uint32_t m = M[i + 1] * m0;

            while (m >= 1024)
            {
                if (out >= out_end) return;
                *out++ = (unsigned char)r;
                r >>= 8;
                m = (m + 255) >> 8;
            }
            R2[i / 2] = r;
            M2[i / 2] = m;
        }

        if (i < len)
        {
            R2[i / 2] = R[i];
            M2[i / 2] = M[i];
        }

        Encode(out, out_end, R2, M2, (len + 1) / 2);
    }
}

void Decode(uint16_t *out, const unsigned char *S, const uint16_t *M, const long long len)
{
    if (len == 1)
    {
        if (M[0] == 1)
            *out = 0;
        else if (M[0] <= 256)
            *out = uint32_mod_uint14(S[0], M[0]);
        else
            *out = uint32_mod_uint14(S[0] + (((uint16_t)S[1]) << 8), M[0]);
    }

    if (len > 1)
    {
        uint16_t R2[(len + 1) / 2];
        uint16_t M2[(len + 1) / 2];
        uint16_t bottomr[len / 2];
        uint32_t bottomt[len / 2];
        long long i;
        for (i = 0; i < len - 1; i += 2)
        {
            uint32_t m = M[i] * (uint32_t)M[i + 1];
            if (m > 256 * 1023)
            {
                bottomt[i / 2] = 256 * 256;
                bottomr[i / 2] = S[0] + 256 * S[1];
                S += 2;
                M2[i / 2] = (((m + 255) >> 8) + 255) >> 8;
            }
            else if (m >= 1024)
            {
                bottomt[i / 2] = 256;
                bottomr[i / 2] = S[0];
                S += 1;
                M2[i / 2] = (m + 255) >> 8;
            }
            else
            {
                bottomt[i / 2] = 1;
                bottomr[i / 2] = 0;
                M2[i / 2] = m;
            }
        }
        if (i < len)
            M2[i / 2] = M[i];

        Decode(R2, S, M2, (len + 1) / 2);

        for (i = 0; i < len - 1; i += 2)
        {
            uint32_t r = bottomr[i / 2];
            uint32_t r1;
            uint16_t r0;
            r += bottomt[i / 2] * R2[i / 2];
            uint32_divmod_uint14(&r1, &r0, r, M[i]);
            r1 = uint32_mod_uint14(r1, M[i + 1]);
            *out++ = r0;
            *out++ = r1;
        }
        if (i < len)
            *out++ = R2[i / 2];
    }
}

void pack_pk(unsigned char *r, const poly *a)
{
    uint16_t R[DTRU_N], M[DTRU_N];

    for (int i = 0; i < DTRU_N; ++i)
        R[i] = (uint16_t)a->coeffs[i];

    for (int i = 0; i < DTRU_N; ++i)
        M[i] = DTRU_Q;

    Encode(r, r + DTRU_PKE_PUBLICKEYBYTES, R, M, DTRU_N);
}

void unpack_pk(poly *r, const unsigned char *a)
{
    uint16_t R[DTRU_N], M[DTRU_N];

    for (int i = 0; i < DTRU_N; ++i)
        M[i] = DTRU_Q;

    Decode(R, a, M, DTRU_N);

    for (int i = 0; i < DTRU_N; ++i)
        r->coeffs[i] = ((int16_t)R[i]);
}

#else

void pack_pk(unsigned char *r, const poly *a)
{
    unsigned int i;
    for (i = 0; i < DTRU_N / 2; i++)
    {
        r[3 * i + 0] = (a->coeffs[2 * i + 0] >> 0);
        r[3 * i + 1] = (a->coeffs[2 * i + 0] >> 8) | (a->coeffs[2 * i + 1] << 4);
        r[3 * i + 2] = (a->coeffs[2 * i + 1] >> 4);
    }
}

void unpack_pk(poly *r, const unsigned char *a)
{
    unsigned int i;
    for (i = 0; i < DTRU_N / 2; i++)
    {
        r->coeffs[2 * i + 0] = ((a[3 * i + 0] >> 0) | ((uint16_t)a[3 * i + 1] << 8)) & 0xFFF;
        r->coeffs[2 * i + 1] = ((a[3 * i + 1] >> 4) | ((uint16_t)a[3 * i + 2] << 4)) & 0xFFF;
    }
}

#endif

void pack_sk(unsigned char *r, const poly *a)
{
    unsigned int i;
    uint8_t t[2];

    for (i = 0; i < DTRU_N / 2; i++)
    {
        t[0] = DTRU_BOUND - a->coeffs[2 * i + 0];
        t[1] = DTRU_BOUND - a->coeffs[2 * i + 1];
        r[i] = (t[0] >> 0) | (t[1] << 4);
    }
}

void unpack_sk(poly *r, const unsigned char *a)
{
    int i;
    for (i = 0; i < DTRU_N / 2; ++i)
    {
        r->coeffs[2 * i + 0] = DTRU_BOUND - ((a[i] >> 0) & 0xF);
        r->coeffs[2 * i + 1] = DTRU_BOUND - ((a[i] >> 4) & 0xF);
    }
}

void pack_ct(unsigned char *r, const poly *a)
{
    unsigned int i;
    for (i = 0; i < DTRU_N / 8; ++i)
    {
        r[9 * i + 0] = (a->coeffs[8 * i + 0] >> 0);
        r[9 * i + 1] = (a->coeffs[8 * i + 0] >> 8) | (a->coeffs[8 * i + 1] << 1);
        r[9 * i + 2] = (a->coeffs[8 * i + 1] >> 7) | (a->coeffs[8 * i + 2] << 2);
        r[9 * i + 3] = (a->coeffs[8 * i + 2] >> 6) | (a->coeffs[8 * i + 3] << 3);
        r[9 * i + 4] = (a->coeffs[8 * i + 3] >> 5) | (a->coeffs[8 * i + 4] << 4);
        r[9 * i + 5] = (a->coeffs[8 * i + 4] >> 4) | (a->coeffs[8 * i + 5] << 5);
        r[9 * i + 6] = (a->coeffs[8 * i + 5] >> 3) | (a->coeffs[8 * i + 6] << 6);
        r[9 * i + 7] = (a->coeffs[8 * i + 6] >> 2) | (a->coeffs[8 * i + 7] << 7);
        r[9 * i + 8] = (a->coeffs[8 * i + 7] >> 1);
    }
}

void unpack_decompress_ct(poly *r, const unsigned char *a)
{
    unsigned int i, j;
    int16_t t[8];
    int32_t temp;
    for (i = 0; i < DTRU_N / 8; ++i)
    {
        t[0] = ((a[9 * i + 0] >> 0) | ((uint16_t)a[9 * i + 1] << 8)) & 0x1FF;
        t[1] = ((a[9 * i + 1] >> 1) | ((uint16_t)a[9 * i + 2] << 7)) & 0x1FF;
        t[2] = ((a[9 * i + 2] >> 2) | ((uint16_t)a[9 * i + 3] << 6)) & 0x1FF;
        t[3] = ((a[9 * i + 3] >> 3) | ((uint16_t)a[9 * i + 4] << 5)) & 0x1FF;
        t[4] = ((a[9 * i + 4] >> 4) | ((uint16_t)a[9 * i + 5] << 4)) & 0x1FF;
        t[5] = ((a[9 * i + 5] >> 5) | ((uint16_t)a[9 * i + 6] << 3)) & 0x1FF;
        t[6] = ((a[9 * i + 6] >> 6) | ((uint16_t)a[9 * i + 7] << 2)) & 0x1FF;
        t[7] = ((a[9 * i + 7] >> 7) | ((uint16_t)a[9 * i + 8] << 1)) & 0x1FF;

        for (j = 0; j < 8; ++j)
        {
            temp = ((int32_t)(t[j] * DTRU_Q) + (DTRU_Q2 >> 1)) >> DTRU_LOGQ2;
            r->coeffs[8 * i + j] = temp;
        }
    }
}
