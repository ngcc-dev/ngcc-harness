#include "vdoo_verif.h"

void vdoo_naive_evaluation(unsigned char *y, const unsigned char *pk, const unsigned char *x)
{
    int monom_idx = 0;

    for (int k = 0; k < VDOO_M; k++)
        y[k] = 0;

    for (int i = 0; i < VDOO_N; i++)
    {
        uint8_t xi = gfv_get_ele(x, i);
        for (int j = i; j < VDOO_N; j++)
        {
            uint8_t xj = gfv_get_ele(x, j);
            uint8_t t = gfv_mul(xi, xj);

            // Adds current Q_i,j * x_i * x_j to each polynomial evaluation in y
            for (int k = 0; k < VDOO_M; k++)
            {
                int elem_idx = monom_idx * VDOO_M + k;
                uint8_t coeff = gfv_get_ele(pk, elem_idx);
                y[k] ^= gfv_mul(coeff, t);
            }
            monom_idx++;
        }
    }
}

void vdoo_optimized_evaluation(unsigned char *y, const unsigned char *pk, const unsigned char *x)
{
    unsigned char tmp[VDOO_Q * VDOO_M] = {0};
    unsigned char _x[VDOO_N];

    for (unsigned i = 0; i < VDOO_N; i++)
        _x[i] = gfv_get_ele(x, i);

    int monom_idx = 0;
    for (unsigned i = 0; i < VDOO_N; i++)
    {
        uint8_t xi = _x[i];
        for (unsigned j = i; j < VDOO_N; j++)
        {
            uint8_t xj = _x[j];
            unsigned char t = gfv_mul(xi, xj);
            // Moves the pointer to row of elements equal to t = xi*xj
            unsigned char *_y = tmp + t * VDOO_M;

            for (unsigned k = 0; k < VDOO_M; k++)
            {
                int elem_idx = monom_idx * VDOO_M + k;

                // gets coefficient of elements w_i*w_j
                uint8_t coeff = gfv_get_ele(pk, elem_idx);

                // Makes sum of coefficient of elements that will be equal to t after evluation
                _y[k] ^= coeff;
            }
            monom_idx++;
        }
    }

    // Could also initialize to zero, but it would add a loop
    // and a useless multiplication.
    // So we initialize with _y1
    unsigned char *_y1 = tmp + 1 * VDOO_M;
    for (unsigned k = 0; k < VDOO_M; k++)
        gfv_set_ele(y, k, _y1[k]);




    for (unsigned t = 2; t < VDOO_Q; t++)
    {
        unsigned char *_y = tmp + t * VDOO_M;
        for (unsigned k = 0; k < VDOO_M; k++)
            gfv_set_ele(y, k, gfv_get_ele(y, k) ^ gfv_mul(_y[k], t));
    }
}


int vdoo_verify(const uint8_t *digest, const uint8_t *signature, const pk_t *pk)
{
    unsigned char computed[VDOO_M];
    unsigned char correct[VDOO_M_BYTE];
    unsigned char digest_salt[HASH_LEN + SALT_BYTES];

    const uint8_t *salt = signature + VDOO_N_BYTE;
    memcpy(digest_salt, digest, HASH_LEN);
    memcpy(digest_salt + HASH_LEN, salt, SALT_BYTES);

    hash_msg(correct, VDOO_M_BYTE, digest_salt, HASH_LEN + SALT_BYTES);

    vdoo_optimized_evaluation(computed, pk->pk, signature);

    unsigned char diff = 0;
    for (int i = 0; i < VDOO_M; i++)
        diff |= (gfv_get_ele(computed, i) ^ gfv_get_ele(correct, i));

    return (diff == 0) ? 0 : -1;
}