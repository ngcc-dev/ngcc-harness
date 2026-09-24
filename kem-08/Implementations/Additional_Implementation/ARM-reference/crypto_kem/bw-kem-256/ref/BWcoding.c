
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "params.h"
#include "BWcoding.h"

#define lambda (1 << KYBER_EQ)

#define mod ((lambda << 2) - 1)


static void mul_phi_inv(int16_t *w, const int16_t *y, const int n)
{
    for (int i = 0; i < n / 2; i++)
    {
        int16_t tmp = y[2 * i];
        w[2 * i]     = (y[2 * i + 1] + tmp) >> 1;  /* (y[2i] + y[2i+1]) / 2 */
        w[2 * i + 1] = (y[2 * i + 1] - tmp) >> 1;  /* (y[2i+1] − y[2i]) / 2 */
    }
}

static void mul_phi(int16_t *w, const int16_t *y, const int n)
{
    for (int i = 0; i < n / 2; i++)
    {
        int16_t tmp = y[2 * i];
        w[2 * i]     = tmp - y[2 * i + 1];  /* a − b */
        w[2 * i + 1] = tmp + y[2 * i + 1];  /* a + b */
    }
}


static void vec_add(int16_t *w, const int16_t *y1, const int16_t *y2, const int n)
{
    for (int i = 0; i < n; i++)
    {
        w[i] = y1[i] + y2[i];
    }
}

static void vec_sub(int16_t *w, const int16_t *y1, const int16_t *y2, const int n)
{
    for (int i = 0; i < n; i++)
    {
        w[i] = y1[i] - y2[i];
    }
}

static uint32_t abs_vec(int16_t *y, const int n)
{
    uint32_t sum = 0;
    for (int i = 0; i < n; i++)
    {
        sum += y[i] * y[i];
    }
    return sum;
}

static uint32_t vec_dis(const int16_t *y, const int16_t *w,
                        const int16_t *t1, const int16_t *t2, const int half_n)
{
    uint32_t dis;
    int16_t tmp[2 * half_n];

    vec_sub(tmp,           y, t1, half_n);   /* y − t1 */
    vec_sub(tmp + half_n,  w, t2, half_n);   /* w − t2 */
    dis = abs_vec(tmp, 2 * half_n);
    return dis;
}

static void BDD_8(int16_t *w, const int16_t *t) {
    int16_t y1_0, y1_1, y1_2, y1_3;
    int16_t y2_0, y2_1, y2_2, y2_3;

    int16_t tmp_z1[4];
    int16_t z1_0, z1_1, z1_2, z1_3;   /* BDD₄(φ⁻¹(t[4..7] − y₁)) */

    int16_t tmp_z2[4];
    int16_t z2_0, z2_1, z2_2, z2_3;   /* BDD₄(φ⁻¹(t[0..3] − y₂)) */

    int16_t w1_0, w1_1, w1_2, w1_3;
    int16_t w2_0, w2_1, w2_2, w2_3;

    {
        int16_t bdd4_y1_0, bdd4_y1_1;   /* BDD₂(t[0..1]) */
        int16_t bdd4_y2_0, bdd4_y2_1;   /* BDD₂(t[2..3]) */
        int16_t bdd4_z1_0, bdd4_z1_1;   /* BDD₂(φ⁻¹(t[2..3] − y₁)) */
        int16_t bdd4_z2_0, bdd4_z2_1;   /* BDD₂(φ⁻¹(t[0..1] − y₂)) */
        int16_t bdd4_w1_0, bdd4_w1_1;
        int16_t bdd4_w2_0, bdd4_w2_1;
        int16_t bdd4_tmp_0, bdd4_tmp_1;
        int16_t bdd4_tmp_phi_0, bdd4_tmp_phi_1;

        bdd4_y1_0 = ((t[0] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_y1_1 = ((t[1] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;

        /* BDD₂(t[2..3]) */
        bdd4_y2_0 = ((t[2] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_y2_1 = ((t[3] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;

        bdd4_tmp_0   = t[2] - bdd4_y1_0;
        bdd4_tmp_1   = t[3] - bdd4_y1_1;
        /* φ⁻¹(tmp)：(tmp[0]+tmp[1])/2, (tmp[1]-tmp[0])/2 */
        bdd4_tmp_phi_0 = (bdd4_tmp_1 + bdd4_tmp_0) >> 1;
        bdd4_tmp_phi_1 = (bdd4_tmp_1 - bdd4_tmp_0) >> 1;
        bdd4_z1_0 = ((bdd4_tmp_phi_0 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_z1_1 = ((bdd4_tmp_phi_1 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;

        bdd4_tmp_0   = t[0] - bdd4_y2_0;
        bdd4_tmp_1   = t[1] - bdd4_y2_1;
        bdd4_tmp_phi_0 = (bdd4_tmp_1 + bdd4_tmp_0) >> 1;
        bdd4_tmp_phi_1 = (bdd4_tmp_1 - bdd4_tmp_0) >> 1;
        bdd4_z2_0 = ((bdd4_tmp_phi_0 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_z2_1 = ((bdd4_tmp_phi_1 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;

        bdd4_w1_0 = bdd4_z1_0 - bdd4_z1_1;
        bdd4_w1_1 = bdd4_z1_0 + bdd4_z1_1;
        bdd4_w1_0 += bdd4_y1_0;
        bdd4_w1_1 += bdd4_y1_1;

        bdd4_w2_0 = bdd4_z2_0 - bdd4_z2_1;
        bdd4_w2_1 = bdd4_z2_0 + bdd4_z2_1;
        bdd4_w2_0 += bdd4_y2_0;
        bdd4_w2_1 += bdd4_y2_1;

        uint32_t bdd4_dis1 = 0, bdd4_dis2 = 0;
        int16_t bdd4_d1_0, bdd4_d1_1, bdd4_d1_2, bdd4_d1_3;
        int16_t bdd4_d2_0, bdd4_d2_1, bdd4_d2_2, bdd4_d2_3;

        bdd4_d1_0 = bdd4_y1_0 - t[0]; bdd4_d1_1 = bdd4_y1_1 - t[1];
        bdd4_d1_2 = bdd4_w1_0 - t[2]; bdd4_d1_3 = bdd4_w1_1 - t[3];
        bdd4_dis1 = bdd4_d1_0*bdd4_d1_0 + bdd4_d1_1*bdd4_d1_1
                  + bdd4_d1_2*bdd4_d1_2 + bdd4_d1_3*bdd4_d1_3;

        bdd4_d2_0 = bdd4_y2_0 - t[2]; bdd4_d2_1 = bdd4_y2_1 - t[3];
        bdd4_d2_2 = bdd4_w2_0 - t[0]; bdd4_d2_3 = bdd4_w2_1 - t[1];
        bdd4_dis2 = bdd4_d2_0*bdd4_d2_0 + bdd4_d2_1*bdd4_d2_1
                  + bdd4_d2_2*bdd4_d2_2 + bdd4_d2_3*bdd4_d2_3;

        uint16_t bdd4_mask = -(((bdd4_dis2 - bdd4_dis1) >> 31) & 1);
        y1_0 = bdd4_y1_0 ^ (bdd4_mask & (bdd4_y1_0 ^ bdd4_w2_0));
        y1_1 = bdd4_y1_1 ^ (bdd4_mask & (bdd4_y1_1 ^ bdd4_w2_1));
        y1_2 = bdd4_w1_0 ^ (bdd4_mask & (bdd4_w1_0 ^ bdd4_y2_0));
        y1_3 = bdd4_w1_1 ^ (bdd4_mask & (bdd4_w1_1 ^ bdd4_y2_1));
    }

    {
        int16_t bdd4_y1_0, bdd4_y1_1, bdd4_y2_0, bdd4_y2_1;
        int16_t bdd4_z1_0, bdd4_z1_1, bdd4_z2_0, bdd4_z2_1;
        int16_t bdd4_w1_0, bdd4_w1_1, bdd4_w2_0, bdd4_w2_1;
        int16_t bdd4_tmp_0, bdd4_tmp_1, bdd4_tmp_phi_0, bdd4_tmp_phi_1;

        bdd4_y1_0 = ((t[4] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_y1_1 = ((t[5] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_y2_0 = ((t[6] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_y2_1 = ((t[7] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;

        bdd4_tmp_0 = t[6] - bdd4_y1_0; bdd4_tmp_1 = t[7] - bdd4_y1_1;
        bdd4_tmp_phi_0 = (bdd4_tmp_1 + bdd4_tmp_0) >> 1;
        bdd4_tmp_phi_1 = (bdd4_tmp_1 - bdd4_tmp_0) >> 1;
        bdd4_z1_0 = ((bdd4_tmp_phi_0 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_z1_1 = ((bdd4_tmp_phi_1 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;

        bdd4_tmp_0 = t[4] - bdd4_y2_0; bdd4_tmp_1 = t[5] - bdd4_y2_1;
        bdd4_tmp_phi_0 = (bdd4_tmp_1 + bdd4_tmp_0) >> 1;
        bdd4_tmp_phi_1 = (bdd4_tmp_1 - bdd4_tmp_0) >> 1;
        bdd4_z2_0 = ((bdd4_tmp_phi_0 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_z2_1 = ((bdd4_tmp_phi_1 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;

        bdd4_w1_0 = bdd4_z1_0 - bdd4_z1_1; bdd4_w1_1 = bdd4_z1_0 + bdd4_z1_1;
        bdd4_w1_0 += bdd4_y1_0;             bdd4_w1_1 += bdd4_y1_1;
        bdd4_w2_0 = bdd4_z2_0 - bdd4_z2_1; bdd4_w2_1 = bdd4_z2_0 + bdd4_z2_1;
        bdd4_w2_0 += bdd4_y2_0;             bdd4_w2_1 += bdd4_y2_1;

        uint32_t bdd4_dis1 = 0, bdd4_dis2 = 0;
        int16_t bdd4_d1_0, bdd4_d1_1, bdd4_d1_2, bdd4_d1_3;
        int16_t bdd4_d2_0, bdd4_d2_1, bdd4_d2_2, bdd4_d2_3;

        bdd4_d1_0 = bdd4_y1_0 - t[4]; bdd4_d1_1 = bdd4_y1_1 - t[5];
        bdd4_d1_2 = bdd4_w1_0 - t[6]; bdd4_d1_3 = bdd4_w1_1 - t[7];
        bdd4_dis1 = bdd4_d1_0*bdd4_d1_0 + bdd4_d1_1*bdd4_d1_1
                  + bdd4_d1_2*bdd4_d1_2 + bdd4_d1_3*bdd4_d1_3;

        bdd4_d2_0 = bdd4_y2_0 - t[6]; bdd4_d2_1 = bdd4_y2_1 - t[7];
        bdd4_d2_2 = bdd4_w2_0 - t[4]; bdd4_d2_3 = bdd4_w2_1 - t[5];
        bdd4_dis2 = bdd4_d2_0*bdd4_d2_0 + bdd4_d2_1*bdd4_d2_1
                  + bdd4_d2_2*bdd4_d2_2 + bdd4_d2_3*bdd4_d2_3;

        uint16_t bdd4_mask = -(((bdd4_dis2 - bdd4_dis1) >> 31) & 1);
        y2_0 = bdd4_y1_0 ^ (bdd4_mask & (bdd4_y1_0 ^ bdd4_w2_0));
        y2_1 = bdd4_y1_1 ^ (bdd4_mask & (bdd4_y1_1 ^ bdd4_w2_1));
        y2_2 = bdd4_w1_0 ^ (bdd4_mask & (bdd4_w1_0 ^ bdd4_y2_0));
        y2_3 = bdd4_w1_1 ^ (bdd4_mask & (bdd4_w1_1 ^ bdd4_y2_1));
    }

    tmp_z1[0] = t[4] - y1_0; tmp_z1[1] = t[5] - y1_1;
    tmp_z1[2] = t[6] - y1_2; tmp_z1[3] = t[7] - y1_3;
    mul_phi_inv(tmp_z1, tmp_z1, 4);

    tmp_z2[0] = t[0] - y2_0; tmp_z2[1] = t[1] - y2_1;
    tmp_z2[2] = t[2] - y2_2; tmp_z2[3] = t[3] - y2_3;
    mul_phi_inv(tmp_z2, tmp_z2, 4);

    {
        int16_t bdd4_y1_0, bdd4_y1_1, bdd4_y2_0, bdd4_y2_1;
        int16_t bdd4_z1_0, bdd4_z1_1, bdd4_z2_0, bdd4_z2_1;
        int16_t bdd4_w1_0, bdd4_w1_1, bdd4_w2_0, bdd4_w2_1;
        int16_t bdd4_tmp_0, bdd4_tmp_1, bdd4_tmp_phi_0, bdd4_tmp_phi_1;

        bdd4_y1_0 = ((tmp_z1[0] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_y1_1 = ((tmp_z1[1] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_y2_0 = ((tmp_z1[2] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_y2_1 = ((tmp_z1[3] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;

        bdd4_tmp_0 = tmp_z1[2] - bdd4_y1_0; bdd4_tmp_1 = tmp_z1[3] - bdd4_y1_1;
        bdd4_tmp_phi_0 = (bdd4_tmp_1 + bdd4_tmp_0) >> 1;
        bdd4_tmp_phi_1 = (bdd4_tmp_1 - bdd4_tmp_0) >> 1;
        bdd4_z1_0 = ((bdd4_tmp_phi_0 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_z1_1 = ((bdd4_tmp_phi_1 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;

        bdd4_tmp_0 = tmp_z1[0] - bdd4_y2_0; bdd4_tmp_1 = tmp_z1[1] - bdd4_y2_1;
        bdd4_tmp_phi_0 = (bdd4_tmp_1 + bdd4_tmp_0) >> 1;
        bdd4_tmp_phi_1 = (bdd4_tmp_1 - bdd4_tmp_0) >> 1;
        bdd4_z2_0 = ((bdd4_tmp_phi_0 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_z2_1 = ((bdd4_tmp_phi_1 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;

        bdd4_w1_0 = bdd4_z1_0 - bdd4_z1_1; bdd4_w1_1 = bdd4_z1_0 + bdd4_z1_1;
        bdd4_w1_0 += bdd4_y1_0;             bdd4_w1_1 += bdd4_y1_1;
        bdd4_w2_0 = bdd4_z2_0 - bdd4_z2_1; bdd4_w2_1 = bdd4_z2_0 + bdd4_z2_1;
        bdd4_w2_0 += bdd4_y2_0;             bdd4_w2_1 += bdd4_y2_1;

        uint32_t bdd4_dis1 = 0, bdd4_dis2 = 0;
        int16_t bdd4_d1_0, bdd4_d1_1, bdd4_d1_2, bdd4_d1_3;
        int16_t bdd4_d2_0, bdd4_d2_1, bdd4_d2_2, bdd4_d2_3;

        bdd4_d1_0 = bdd4_y1_0 - tmp_z1[0]; bdd4_d1_1 = bdd4_y1_1 - tmp_z1[1];
        bdd4_d1_2 = bdd4_w1_0 - tmp_z1[2]; bdd4_d1_3 = bdd4_w1_1 - tmp_z1[3];
        bdd4_dis1 = bdd4_d1_0*bdd4_d1_0 + bdd4_d1_1*bdd4_d1_1
                  + bdd4_d1_2*bdd4_d1_2 + bdd4_d1_3*bdd4_d1_3;

        bdd4_d2_0 = bdd4_y2_0 - tmp_z1[2]; bdd4_d2_1 = bdd4_y2_1 - tmp_z1[3];
        bdd4_d2_2 = bdd4_w2_0 - tmp_z1[0]; bdd4_d2_3 = bdd4_w2_1 - tmp_z1[1];
        bdd4_dis2 = bdd4_d2_0*bdd4_d2_0 + bdd4_d2_1*bdd4_d2_1
                  + bdd4_d2_2*bdd4_d2_2 + bdd4_d2_3*bdd4_d2_3;

        uint16_t bdd4_mask = -(((bdd4_dis2 - bdd4_dis1) >> 31) & 1);
        z1_0 = bdd4_y1_0 ^ (bdd4_mask & (bdd4_y1_0 ^ bdd4_w2_0));
        z1_1 = bdd4_y1_1 ^ (bdd4_mask & (bdd4_y1_1 ^ bdd4_w2_1));
        z1_2 = bdd4_w1_0 ^ (bdd4_mask & (bdd4_w1_0 ^ bdd4_y2_0));
        z1_3 = bdd4_w1_1 ^ (bdd4_mask & (bdd4_w1_1 ^ bdd4_y2_1));
    }

    {
        int16_t bdd4_y1_0, bdd4_y1_1, bdd4_y2_0, bdd4_y2_1;
        int16_t bdd4_z1_0, bdd4_z1_1, bdd4_z2_0, bdd4_z2_1;
        int16_t bdd4_w1_0, bdd4_w1_1, bdd4_w2_0, bdd4_w2_1;
        int16_t bdd4_tmp_0, bdd4_tmp_1, bdd4_tmp_phi_0, bdd4_tmp_phi_1;

        bdd4_y1_0 = ((tmp_z2[0] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_y1_1 = ((tmp_z2[1] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_y2_0 = ((tmp_z2[2] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_y2_1 = ((tmp_z2[3] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;

        bdd4_tmp_0 = tmp_z2[2] - bdd4_y1_0; bdd4_tmp_1 = tmp_z2[3] - bdd4_y1_1;
        bdd4_tmp_phi_0 = (bdd4_tmp_1 + bdd4_tmp_0) >> 1;
        bdd4_tmp_phi_1 = (bdd4_tmp_1 - bdd4_tmp_0) >> 1;
        bdd4_z1_0 = ((bdd4_tmp_phi_0 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_z1_1 = ((bdd4_tmp_phi_1 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;

        bdd4_tmp_0 = tmp_z2[0] - bdd4_y2_0; bdd4_tmp_1 = tmp_z2[1] - bdd4_y2_1;
        bdd4_tmp_phi_0 = (bdd4_tmp_1 + bdd4_tmp_0) >> 1;
        bdd4_tmp_phi_1 = (bdd4_tmp_1 - bdd4_tmp_0) >> 1;
        bdd4_z2_0 = ((bdd4_tmp_phi_0 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
        bdd4_z2_1 = ((bdd4_tmp_phi_1 + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;

        bdd4_w1_0 = bdd4_z1_0 - bdd4_z1_1; bdd4_w1_1 = bdd4_z1_0 + bdd4_z1_1;
        bdd4_w1_0 += bdd4_y1_0;             bdd4_w1_1 += bdd4_y1_1;
        bdd4_w2_0 = bdd4_z2_0 - bdd4_z2_1; bdd4_w2_1 = bdd4_z2_0 + bdd4_z2_1;
        bdd4_w2_0 += bdd4_y2_0;             bdd4_w2_1 += bdd4_y2_1;

        uint32_t bdd4_dis1 = 0, bdd4_dis2 = 0;
        int16_t bdd4_d1_0, bdd4_d1_1, bdd4_d1_2, bdd4_d1_3;
        int16_t bdd4_d2_0, bdd4_d2_1, bdd4_d2_2, bdd4_d2_3;

        bdd4_d1_0 = bdd4_y1_0 - tmp_z2[0]; bdd4_d1_1 = bdd4_y1_1 - tmp_z2[1];
        bdd4_d1_2 = bdd4_w1_0 - tmp_z2[2]; bdd4_d1_3 = bdd4_w1_1 - tmp_z2[3];
        bdd4_dis1 = bdd4_d1_0*bdd4_d1_0 + bdd4_d1_1*bdd4_d1_1
                  + bdd4_d1_2*bdd4_d1_2 + bdd4_d1_3*bdd4_d1_3;

        bdd4_d2_0 = bdd4_y2_0 - tmp_z2[2]; bdd4_d2_1 = bdd4_y2_1 - tmp_z2[3];
        bdd4_d2_2 = bdd4_w2_0 - tmp_z2[0]; bdd4_d2_3 = bdd4_w2_1 - tmp_z2[1];
        bdd4_dis2 = bdd4_d2_0*bdd4_d2_0 + bdd4_d2_1*bdd4_d2_1
                  + bdd4_d2_2*bdd4_d2_2 + bdd4_d2_3*bdd4_d2_3;

        uint16_t bdd4_mask = -(((bdd4_dis2 - bdd4_dis1) >> 31) & 1);
        z2_0 = bdd4_y1_0 ^ (bdd4_mask & (bdd4_y1_0 ^ bdd4_w2_0));
        z2_1 = bdd4_y1_1 ^ (bdd4_mask & (bdd4_y1_1 ^ bdd4_w2_1));
        z2_2 = bdd4_w1_0 ^ (bdd4_mask & (bdd4_w1_0 ^ bdd4_y2_0));
        z2_3 = bdd4_w1_1 ^ (bdd4_mask & (bdd4_w1_1 ^ bdd4_y2_1));
    }


    w1_0 = z1_0 - z1_1;  w1_1 = z1_0 + z1_1;
    w1_2 = z1_2 - z1_3;  w1_3 = z1_2 + z1_3;
    w1_0 += y1_0; w1_1 += y1_1; w1_2 += y1_2; w1_3 += y1_3;

    w2_0 = z2_0 - z2_1;  w2_1 = z2_0 + z2_1;
    w2_2 = z2_2 - z2_3;  w2_3 = z2_2 + z2_3;
    w2_0 += y2_0; w2_1 += y2_1; w2_2 += y2_2; w2_3 += y2_3;

    uint32_t dis1 = 0, dis2 = 0;
    int16_t diff;

    diff = y1_0 - t[0]; dis1 += diff * diff;
    diff = y1_1 - t[1]; dis1 += diff * diff;
    diff = y1_2 - t[2]; dis1 += diff * diff;
    diff = y1_3 - t[3]; dis1 += diff * diff;
    diff = w1_0 - t[4]; dis1 += diff * diff;
    diff = w1_1 - t[5]; dis1 += diff * diff;
    diff = w1_2 - t[6]; dis1 += diff * diff;
    diff = w1_3 - t[7]; dis1 += diff * diff;

    diff = y2_0 - t[4]; dis2 += diff * diff;
    diff = y2_1 - t[5]; dis2 += diff * diff;
    diff = y2_2 - t[6]; dis2 += diff * diff;
    diff = y2_3 - t[7]; dis2 += diff * diff;
    diff = w2_0 - t[0]; dis2 += diff * diff;
    diff = w2_1 - t[1]; dis2 += diff * diff;
    diff = w2_2 - t[2]; dis2 += diff * diff;
    diff = w2_3 - t[3]; dis2 += diff * diff;

    uint16_t mask = -(((dis2 - dis1) >> 31) & 1);
    w[0] = y1_0 ^ (mask & (y1_0 ^ w2_0));
    w[1] = y1_1 ^ (mask & (y1_1 ^ w2_1));
    w[2] = y1_2 ^ (mask & (y1_2 ^ w2_2));
    w[3] = y1_3 ^ (mask & (y1_3 ^ w2_3));
    w[4] = w1_0 ^ (mask & (w1_0 ^ y2_0));
    w[5] = w1_1 ^ (mask & (w1_1 ^ y2_1));
    w[6] = w1_2 ^ (mask & (w1_2 ^ y2_2));
    w[7] = w1_3 ^ (mask & (w1_3 ^ y2_3));
}

static void BDD_16(int16_t *w, const int16_t *t) {
    int16_t y1[8], y2[8];
    int16_t z1[8], z2[8];
    int16_t w1[8], w2[8];
    int16_t tmp[8];

    BDD_8(y1, t);
    BDD_8(y2, t + 8);

    vec_sub(tmp, t + 8, y1, 8);   /* tmp = t[8..15] − y₁ */
    mul_phi_inv(tmp, tmp, 8);      /* tmp = φ⁻¹(t[8..15] − y₁) */
    BDD_8(z1, tmp);                /* z₁ = BDD₈(φ⁻¹(t[8..15] − y₁)) */

    vec_sub(tmp, t, y2, 8);        /* tmp = t[0..7] − y₂ */
    mul_phi_inv(tmp, tmp, 8);
    BDD_8(z2, tmp);                /* z₂ = BDD₈(φ⁻¹(t[0..7] − y₂)) */

    mul_phi(w1, z1, 8);            /* w₁ = φ(z₁) */
    vec_add(w1, y1, w1, 8);        /* w₁ += y₁ */

    mul_phi(w2, z2, 8);
    vec_add(w2, y2, w2, 8);

    uint32_t dis1 = vec_dis(y1, w1, t, t + 8, 8);
    uint32_t dis2 = vec_dis(y2, w2, t + 8, t, 8);

    uint16_t mask = -(((dis2 - dis1) >> 31) & 1);
    for (int i = 0; i < 8; i++) {
        w[i]     = y1[i] ^ (mask & (y1[i] ^ w2[i]));
        w[i + 8] = w1[i] ^ (mask & (w1[i] ^ y2[i]));
    }
}

static void BDD(int16_t *w, const int16_t *t) {
    int16_t y1[16], y2[16];
    int16_t z1[16], z2[16];
    int16_t w1[16], w2[16];
    int16_t tmp[16];

    BDD_16(y1, t);
    BDD_16(y2, t + 16);

    vec_sub(tmp, t + 16, y1, 16);
    mul_phi_inv(tmp, tmp, 16);
    BDD_16(z1, tmp);         /* z₁ = BDD₁₆(φ⁻¹(t[16..31] − y₁)) */

    vec_sub(tmp, t, y2, 16);
    mul_phi_inv(tmp, tmp, 16);
    BDD_16(z2, tmp);         /* z₂ = BDD₁₆(φ⁻¹(t[0..15] − y₂)) */

    mul_phi(w1, z1, 16);
    vec_add(w1, y1, w1, 16);
    mul_phi(w2, z2, 16);
    vec_add(w2, y2, w2, 16);

    uint32_t dis1 = vec_dis(y1, w1, t, t + 16, 16);
    uint32_t dis2 = vec_dis(y2, w2, t + 16, t, 16);

    uint16_t mask = -(((dis2 - dis1) >> 31) & 1);
    for (int i = 0; i < 16; i++) {
        w[i]      = y1[i] ^ (mask & (y1[i] ^ w2[i]));
        w[i + 16] = w1[i] ^ (mask & (w1[i] ^ y2[i]));
    }
}

static uint32_t delabel_bw32(int16_t w[32])
{
    uint64_t t = 0;
    uint32_t s = 0;
    uint8_t mvec[32], vec[32];
    int16_t tmp, cnt = 0;

    for (int i = 0; i < 32; i++){
        vec[i] = (uint8_t) ((w[i] & mod) >> KYBER_EQ);
    }

    mvec[0]  = (uint8_t) vec[0] & 3;
    mvec[1]  = (uint8_t) vec[1] & 3;
    mvec[2]  = (uint8_t) ((vec[2]  + vec[3]  - vec[0]  - vec[1]) >> 1) & 3;
    mvec[3]  = (uint8_t) ((vec[0]  + vec[3]  - vec[1]  - vec[2]) >> 1) & 3;
    mvec[4]  = (uint8_t) ((vec[4]  + vec[5]  - vec[0]  - vec[1]) >> 1) & 3;
    mvec[5]  = (uint8_t) ((vec[0]  + vec[5]  - vec[1]  - vec[4]) >> 1) & 3;
    mvec[6]  = (uint8_t) ((vec[1]  + vec[7]  - vec[3]  - vec[5]) >> 1) & 3;
    mvec[7]  = (uint8_t) ((vec[2]  + vec[4]  - vec[0]  - vec[6]) >> 1) & 3;
    mvec[8]  = (uint8_t) ((vec[8]  + vec[9]  - vec[0]  - vec[1]) >> 1) & 3;
    mvec[9]  = (uint8_t) ((vec[0]  + vec[9]  - vec[1]  - vec[8]) >> 1) & 3;
    mvec[10] = (uint8_t) ((vec[1]  + vec[11] - vec[3]  - vec[9]) >> 1) & 3;
    mvec[11] = (uint8_t) ((vec[2]  + vec[8]  - vec[0]  - vec[10]) >> 1) & 3;
    mvec[12] = (uint8_t) ((vec[1]  + vec[13] - vec[5]  - vec[9]) >> 1) & 3;
    mvec[13] = (uint8_t) ((vec[4]  + vec[8]  - vec[0]  - vec[12]) >> 1) & 3;
    mvec[14] = (uint8_t) ((vec[0]  + vec[3]  + vec[5]  + vec[6]  + vec[9]  + vec[10] + vec[12] + vec[15]
                          - vec[1]  - vec[2]  - vec[4]  - vec[7]  - vec[8]  - vec[11] - vec[13] - vec[14]) >> 2) & 3;
    mvec[15] = (uint8_t) ((vec[0]  + vec[1]  + vec[6]  + vec[7]  + vec[10] + vec[11] + vec[12] + vec[13]
                          - vec[2]  - vec[3]  - vec[4]  - vec[5]  - vec[8]  - vec[9]  - vec[14] - vec[15]) >> 2) & 3;
    mvec[16] = (uint8_t) ((vec[16] + vec[17] - vec[0]  - vec[1]) >> 1) & 3;
    mvec[17] = (uint8_t) ((vec[0]  + vec[17] - vec[1]  - vec[16]) >> 1) & 3;
    mvec[18] = (uint8_t) ((vec[1]  + vec[19] - vec[3]  - vec[17]) >> 1) & 3;
    mvec[19] = (uint8_t) ((vec[2]  + vec[16] - vec[0]  - vec[18]) >> 1) & 3;
    mvec[20] = (uint8_t) ((vec[1]  + vec[21] - vec[5]  - vec[17]) >> 1) & 3;
    mvec[21] = (uint8_t) ((vec[4]  + vec[16] - vec[0]  - vec[20]) >> 1) & 3;
    mvec[22] = (uint8_t) ((vec[0]  + vec[3]  + vec[5]  + vec[6]  + vec[17] + vec[18] + vec[20] + vec[23]
                          - vec[1]  - vec[2]  - vec[4]  - vec[7]  - vec[16] - vec[19] - vec[21] - vec[22]) >> 2) & 3;
    mvec[23] = (uint8_t) ((vec[0]  + vec[1]  + vec[6]  + vec[7]  + vec[18] + vec[19] + vec[20] + vec[21]
                          - vec[2]  - vec[3]  - vec[4]  - vec[5]  - vec[16] - vec[17] - vec[22] - vec[23]) >> 2) & 3;
    mvec[24] = (uint8_t) ((vec[1]  + vec[25] - vec[9]  - vec[17]) >> 1) & 3;
    mvec[25] = (uint8_t) ((vec[8]  + vec[16] - vec[0]  - vec[24]) >> 1) & 3;
    mvec[26] = (uint8_t) ((vec[0]  + vec[3]  + vec[9]  + vec[10] + vec[17] + vec[18] + vec[24] + vec[27]
                          - vec[1]  - vec[2]  - vec[8]  - vec[11] - vec[16] - vec[19] - vec[25] - vec[26]) >> 2) & 3;
    mvec[27] = (uint8_t) ((vec[0]  + vec[1]  + vec[10] + vec[11] + vec[18] + vec[19] + vec[24] + vec[25]
                          - vec[2]  - vec[3]  - vec[8]  - vec[9]  - vec[16] - vec[17] - vec[26] - vec[27]) >> 2) & 3;
    mvec[28] = (uint8_t) ((vec[0]  + vec[5]  + vec[9]  + vec[12] + vec[17] + vec[20] + vec[24] + vec[29]
                          - vec[1]  - vec[4]  - vec[8]  - vec[13] - vec[16] - vec[21] - vec[25] - vec[28]) >> 2) & 3;
    mvec[29] = (uint8_t) ((vec[0]  + vec[1]  + vec[12] + vec[13] + vec[20] + vec[21] + vec[24] + vec[25]
                          - vec[4]  - vec[5]  - vec[8]  - vec[9]  - vec[16] - vec[17] - vec[28] - vec[29]) >> 2) & 3;
    mvec[30] = (uint8_t) ((vec[2]  + vec[4]  + vec[8]  + vec[14] + vec[16] + vec[22] + vec[26] + vec[28]
                          - vec[0]  - vec[6]  - vec[10] - vec[12] - vec[18] - vec[20] - vec[24] - vec[30]) >> 2) & 3;
    mvec[31] = (uint8_t) ((vec[3]  + vec[5]  + vec[9]  + vec[15] + vec[17] + vec[23] + vec[27] + vec[29]
                          - vec[1]  - vec[7]  - vec[11] - vec[13] - vec[19] - vec[21] - vec[25] - vec[31]) >> 2) & 3;

    for (int i = 0; i < 32; i++)
    {
        t |= (uint64_t)(mvec[31 - i]) << (2 * i);
    }

    t = (t & 0xfdd5d554d5545440) ^ ((t & 0x0220200120010110) << 2);

    for (int i = 0; i < 64; i++)
    {
        tmp = (0xfdd5d554d5545440 >> i) & 1;
        s += (-tmp) & (((t >> i) & 1) << cnt);
        cnt += tmp;
    }
    return s;
}

uint32_t decode_bw32(int16_t t[32])
{
    int16_t w[32];
    uint32_t res;

    for (int i = 0; i < 32; i++) {
        t[i] = t[i] << 2;
    }

    BDD(w, t);
    res = delabel_bw32(w);

    return res;
}

uint64_t encode_bw32(uint32_t m)
{
    uint64_t mh, mask, t, high = 0, low = 0;

    uint64_t bw_high[32] = {
        0x00000000000000aa, 0x0000000000000a0a, 0x0000000000008888,
        0x0000000000002222, 0x00000000000a000a, 0x0000000000880088,
        0x0000000000220022, 0x0000000008080808, 0x0000000002020202,
        0x0000000088888888, 0x0000000000000000, 0x00000000aaaaaaaa,
        0x0000000a0000000a, 0x0000008800000088, 0x0000002200000022,
        0x0000080800000808, 0x0000020200000202, 0x0000888800008888,
        0x0000000000000000, 0x0000aaaa0000aaaa, 0x0008000800080008,
        0x0002000200020002, 0x0088008800880088, 0x0000000000000000,
        0x00aa00aa00aa00aa, 0x0808080808080808, 0x0000000000000000,
        0x0a0a0a0a0a0a0a0a, 0x0000000000000000, 0x2222222222222222,
        0x0000000000000000, 0x8888888888888888
    };
    uint64_t bw_low[32] = {
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
        0x0000000055555555, 0x0000000055555555, 0x0000000000000000,
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
        0x0000000000000000, 0x0000000000000000, 0x0000555500005555,
        0x0000555500005555, 0x0000000000000000, 0x0000000000000000,
        0x0000000000000000, 0x0055005500550055, 0x0055005500550055,
        0x0000000000000000, 0x0505050505050505, 0x0505050505050505,
        0x0000000000000000, 0x1111111111111111, 0x0000000000000000,
        0x4444444444444444, 0x0000000000000000
    };

    for (int i = 0; i < 32; i++)
    {
        mask = (m >> i) & 1;
        mask = -mask;

        high ^= mask & bw_high[i];

        t    = low + (mask & bw_low[i]);
        low  = t & 0x5555555555555555;
        high ^= t & 0xaaaaaaaaaaaaaaaa;
    }

    mh = high | low;
    return mh;
}
