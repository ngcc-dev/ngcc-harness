#include "conversion.h"

/*************************************************
* Name:        SimpleBitPack
*
* Description: BitPack polynomial with coefficients in [0,b].
*
* Arguments:   - uint8_t *z: pointer to output byte array
*              - const poly *w: pointer to input polynomial
*              - int b: pointer to input Pack number
**************************************************/
void SimpleBitPack(uint8_t* z, const poly* w, int b)
{
    int i, j;

    if (b == 1)
    {
        for (i = 0; i < n / 8; i++)
        {
            z[i] = 0;
            for (j = 0; j < 8; j++) {
                z[i] |= w->coeffs[8 * i + j] << j;
            }
        }
    }
    else if (b == 4)
    {
        for (i = 0; i < n / 2; ++i)
            z[i] = w->coeffs[2 * i] | (w->coeffs[2 * i + 1] << 4);
    }
    else if (b == 5)
    {
        for (i = 0; i < n / 8; i++)
        {
            z[0] = (w->coeffs[8 * i + 0] >> 0) | (w->coeffs[8 * i + 1] << 5);
            z[1] = (w->coeffs[8 * i + 1] >> 3) | (w->coeffs[8 * i + 2] << 2) | (w->coeffs[8 * i + 3] << 7);
            z[2] = (w->coeffs[8 * i + 3] >> 1) | (w->coeffs[8 * i + 4] << 4);
            z[3] = (w->coeffs[8 * i + 4] >> 4) | (w->coeffs[8 * i + 5] << 1) | (w->coeffs[8 * i + 6] << 6);
            z[4] = (w->coeffs[8 * i + 6] >> 2) | (w->coeffs[8 * i + 7] << 3);
            z += 5;
        }
    }
    else if (b == 9)
    {
        for (i = 0; i < n / 8; i++)
        {
            z[0] = (w->coeffs[8 * i + 0] >> 0);
            z[1] = (w->coeffs[8 * i + 0] >> 8) | (w->coeffs[8 * i + 1] << 1);
            z[2] = (w->coeffs[8 * i + 1] >> 7) | (w->coeffs[8 * i + 2] << 2);
            z[3] = (w->coeffs[8 * i + 2] >> 6) | (w->coeffs[8 * i + 3] << 3);
            z[4] = (w->coeffs[8 * i + 3] >> 5) | (w->coeffs[8 * i + 4] << 4);
            z[5] = (w->coeffs[8 * i + 4] >> 4) | (w->coeffs[8 * i + 5] << 5);
            z[6] = (w->coeffs[8 * i + 5] >> 3) | (w->coeffs[8 * i + 6] << 6);
            z[7] = (w->coeffs[8 * i + 6] >> 2) | (w->coeffs[8 * i + 7] << 7);
            z[8] = (w->coeffs[8 * i + 7] >> 1);
            z += 9;
        }
    }
    else if (b == 10)
    {
        for (i = 0; i < n / 4; ++i)
        {
            z[0] = (w->coeffs[4 * i + 0] >> 0);
            z[1] = (w->coeffs[4 * i + 0] >> 8) | (w->coeffs[4 * i + 1] << 2);
            z[2] = (w->coeffs[4 * i + 1] >> 6) | (w->coeffs[4 * i + 2] << 4);
            z[3] = (w->coeffs[4 * i + 2] >> 4) | (w->coeffs[4 * i + 3] << 6);
            z[4] = (w->coeffs[4 * i + 3] >> 2);
            z += 5;
        }
    }
    else if (b == 11)
    {
        for (i = 0; i < n / 8; i++)
        {
            z[0] = (w->coeffs[8 * i + 0] >> 0);
            z[1] = (w->coeffs[8 * i + 0] >> 8) | (w->coeffs[8 * i + 1] << 3);
            z[2] = (w->coeffs[8 * i + 1] >> 5) | (w->coeffs[8 * i + 2] << 6);
            z[3] = (w->coeffs[8 * i + 2] >> 2);
            z[4] = (w->coeffs[8 * i + 2] >> 10) | (w->coeffs[8 * i + 3] << 1);
            z[5] = (w->coeffs[8 * i + 3] >> 7) | (w->coeffs[8 * i + 4] << 4);
            z[6] = (w->coeffs[8 * i + 4] >> 4) | (w->coeffs[8 * i + 5] << 7);
            z[7] = (w->coeffs[8 * i + 5] >> 1);
            z[8] = (w->coeffs[8 * i + 5] >> 9) | (w->coeffs[8 * i + 6] << 2);
            z[9] = (w->coeffs[8 * i + 6] >> 6) | (w->coeffs[8 * i + 7] << 5);
            z[10] = (w->coeffs[8 * i + 7] >> 3);
            z += 11;
        }
    }
    else
    {
        printf("SimpleBitPack error: b must be in { 1,4,5,9,10,11 }!\n");
        exit(0);
    }
}

/*************************************************
* Name:        SimpleBitUnpack
*
* Description: Unpack polynomial with coefficients in [0,b].
*
* Arguments:   - poly *w: pointer to output polynomial
*              - const uint8_t *z: byte array with bit-packed polynomial
*              - int b: pointer to input Unpack number
**************************************************/
void SimpleBitUnpack(poly* w, const uint8_t* z, int b)
{
    int i;

    if (b == 9)
    {
        for (i = 0; i < n / 8; i++)
        {
            w->coeffs[8 * i + 0] = ((z[0] >> 0) | ((int32_t)z[1] << 8)) & 0x1FF;
            w->coeffs[8 * i + 1] = ((z[1] >> 1) | ((int32_t)z[2] << 7)) & 0x1FF;
            w->coeffs[8 * i + 2] = ((z[2] >> 2) | ((int32_t)z[3] << 6)) & 0x1FF;
            w->coeffs[8 * i + 3] = ((z[3] >> 3) | ((int32_t)z[4] << 5)) & 0x1FF;
            w->coeffs[8 * i + 4] = ((z[4] >> 4) | ((int32_t)z[5] << 4)) & 0x1FF;
            w->coeffs[8 * i + 5] = ((z[5] >> 5) | ((int32_t)z[6] << 3)) & 0x1FF;
            w->coeffs[8 * i + 6] = ((z[6] >> 6) | ((int32_t)z[7] << 2)) & 0x1FF;
            w->coeffs[8 * i + 7] = ((z[7] >> 7) | ((int32_t)z[8] << 1)) & 0x1FF;
            z += 9;
        }
    }
    else if (b == 10)
    {
        for (i = 0; i < n / 4; ++i) {
            w->coeffs[4 * i + 0] = ((z[0] >> 0) | ((int32_t)z[1] << 8)) & 0x3FF;
            w->coeffs[4 * i + 1] = ((z[1] >> 2) | ((int32_t)z[2] << 6)) & 0x3FF;
            w->coeffs[4 * i + 2] = ((z[2] >> 4) | ((int32_t)z[3] << 4)) & 0x3FF;
            w->coeffs[4 * i + 3] = ((z[3] >> 6) | ((int32_t)z[4] << 2)) & 0x3FF;
            z += 5;
        }
    }
    else if (b == 11)
    {
        for (i = 0; i < n / 8; i++)
        {
            w->coeffs[8 * i + 0] = ((z[0] >> 0) | ((int32_t)z[1] << 8)) & 0x7FF;
            w->coeffs[8 * i + 1] = ((z[1] >> 3) | ((int32_t)z[2] << 5)) & 0x7FF;
            w->coeffs[8 * i + 2] = ((z[2] >> 6) | ((int32_t)z[3] << 2) | ((int32_t)z[4] << 10)) & 0x7FF;
            w->coeffs[8 * i + 3] = ((z[4] >> 1) | ((int32_t)z[5] << 7)) & 0x7FF;
            w->coeffs[8 * i + 4] = ((z[5] >> 4) | ((int32_t)z[6] << 4)) & 0x7FF;
            w->coeffs[8 * i + 5] = ((z[6] >> 7) | ((int32_t)z[7] << 1) | ((int32_t)z[8] << 9)) & 0x7FF;
            w->coeffs[8 * i + 6] = ((z[8] >> 2) | ((int32_t)z[9] << 6)) & 0x7FF;
            w->coeffs[8 * i + 7] = ((z[9] >> 5) | ((int32_t)z[10] << 3)) & 0x7FF;
            z += 11;
        }
    }
    else
    {
        printf("SimpleBitUnpack error: b must be in { 9,10,11 }!\n");
        exit(0);
    }
}

/*************************************************
* Name:        BitPack
*
* Description: BitPack polynomial with coefficients in [-b,b].
*
* Arguments:   - uint8_t *z: pointer to output byte array
*              - const poly *w: pointer to input polynomial
*              - int b: pointer to input Pack number
**************************************************/
void BitPack(uint8_t* z, const poly* w, int b)
{
    int32_t i, t[8];

    if (b == 1 || b == 2)
    {
        for (i = 0; i < n / 4; ++i)
        {
            t[0] = b - w->coeffs[4 * i + 0];
            t[1] = b - w->coeffs[4 * i + 1];
            t[2] = b - w->coeffs[4 * i + 2];
            t[3] = b - w->coeffs[4 * i + 3];

            z[i] = (t[0] << 0) | (t[1] << 2) | (t[2] << 4) | (t[3] << 6);
        }
    }
    else if (b == 5)
    {
        for (i = 0; i < n / 8; ++i)
        {
            t[0] = 16 - w->coeffs[8 * i + 0];
            t[1] = 16 - w->coeffs[8 * i + 1];
            t[2] = 16 - w->coeffs[8 * i + 2];
            t[3] = 16 - w->coeffs[8 * i + 3];
            t[4] = 16 - w->coeffs[8 * i + 4];
            t[5] = 16 - w->coeffs[8 * i + 5];
            t[6] = 16 - w->coeffs[8 * i + 6];
            t[7] = 16 - w->coeffs[8 * i + 7];
            z[0] = (t[0] >> 0) | (t[1] << 5);
            z[1] = (t[1] >> 3) | (t[2] << 2) | (t[3] << 7);
            z[2] = (t[3] >> 1) | (t[4] << 4);
            z[3] = (t[4] >> 4) | (t[5] << 1) | (t[6] << 6);
            z[4] = (t[6] >> 2) | (t[7] << 3);
            z += 5;
        }
    }
    else if (b == 6)
    {
        for (i = 0; i < n / 4; ++i)
        {
            t[0] = 32 - w->coeffs[4 * i + 0];
            t[1] = 32 - w->coeffs[4 * i + 1];
            t[2] = 32 - w->coeffs[4 * i + 2];
            t[3] = 32 - w->coeffs[4 * i + 3];
            z[0] = t[0] | (t[1] << 6);
            z[1] = t[1] >> 2 | (t[2] << 4);
            z[2] = t[2] >> 4 | (t[3] << 2);
            z += 3;
        }
    }
    else if (b == 7)
    {
        for (i = 0; i < n / 8; ++i)
        {
            t[0] = 64 - w->coeffs[8 * i + 0];
            t[1] = 64 - w->coeffs[8 * i + 1];
            t[2] = 64 - w->coeffs[8 * i + 2];
            t[3] = 64 - w->coeffs[8 * i + 3];
            t[4] = 64 - w->coeffs[8 * i + 4];
            t[5] = 64 - w->coeffs[8 * i + 5];
            t[6] = 64 - w->coeffs[8 * i + 6];
            t[7] = 64 - w->coeffs[8 * i + 7];
            z[0] = t[0] | (t[1] << 7);
            z[1] = t[1] >> 1 | (t[2] << 6);
            z[2] = t[2] >> 2 | (t[3] << 5);
            z[3] = t[3] >> 3 | (t[4] << 4);
            z[4] = t[4] >> 4 | (t[5] << 3);
            z[5] = t[5] >> 5 | (t[6] << 2);
            z[6] = t[6] >> 6 | (t[7] << 1);
            z += 7;
        }
    }
    else
    {
        printf("BitPack error: b must be in{ 1,2,5,6,7 }!\n");
        exit(0);
    }
}

/*************************************************
* Name:        BitUnpack
*
* Description: Unpack polynomial with coefficients in [-b,b].
*
* Arguments:   - poly *w: pointer to output polynomial
*              - const uint8_t *z: byte array with bit-packed polynomial
*              - int b: pointer to input Unpack number
**************************************************/
void BitUnpack(poly* w, const uint8_t* z, int b)
{
    int i;

    if (b == 1 || b == 2)
    {
        for (i = 0; i < n / 4; ++i)
        {
            w->coeffs[4 * i + 0] = b - (z[i] & 3);
            w->coeffs[4 * i + 1] = b - ((z[i] >> 2) & 3);
            w->coeffs[4 * i + 2] = b - ((z[i] >> 4) & 3);
            w->coeffs[4 * i + 3] = b - (z[i] >> 6);
        }
    }
    else if (b == 5)
    {
        for (i = 0; i < n / 8; ++i)
        {
            w->coeffs[8 * i + 0] = 16 - (z[0] & 0x1F);
            w->coeffs[8 * i + 1] = 16 - (((z[0] >> 5) | (z[1] << 3)) & 0x1F);
            w->coeffs[8 * i + 2] = 16 - (((z[1] >> 2)) & 0x1F);
            w->coeffs[8 * i + 3] = 16 - (((z[1] >> 7) | (z[2] << 1)) & 0x1F);
            w->coeffs[8 * i + 4] = 16 - (((z[2] >> 4) | (z[3] << 4)) & 0x1F);
            w->coeffs[8 * i + 5] = 16 - (((z[3] >> 1)) & 0x1F);
            w->coeffs[8 * i + 6] = 16 - (((z[3] >> 6) | (z[4] << 2)) & 0x1F);
            w->coeffs[8 * i + 7] = 16 - (z[4] >> 3);
            z += 5;
        }
    }
    else if (b == 6)
    {
        for (i = 0; i < n / 4; ++i)
        {
            w->coeffs[4 * i + 0] = 32 - (z[0] & 0x3F);
            w->coeffs[4 * i + 1] = 32 - (((z[0] >> 6) | ((int32_t)z[1] << 2)) & 0x3F);
            w->coeffs[4 * i + 2] = 32 - (((z[1] >> 4) | ((int32_t)z[2] << 4)) & 0x3F);
            w->coeffs[4 * i + 3] = 32 - (z[2] >> 2);
            z += 3;
        }
    }
    else if (b == 7)
    {
        for (i = 0; i < n / 8; ++i)
        {
            w->coeffs[8 * i + 0] = 64 - (z[0] & 0x7F);
            w->coeffs[8 * i + 1] = 64 - (((z[0] >> 7) | ((int32_t)z[1] << 1)) & 0x7F);
            w->coeffs[8 * i + 2] = 64 - (((z[1] >> 6) | ((int32_t)z[2] << 2)) & 0x7F);
            w->coeffs[8 * i + 3] = 64 - (((z[2] >> 5) | ((int32_t)z[3] << 3)) & 0x7F);
            w->coeffs[8 * i + 4] = 64 - (((z[3] >> 4) | ((int32_t)z[4] << 4)) & 0x7F);
            w->coeffs[8 * i + 5] = 64 - (((z[4] >> 3) | ((int32_t)z[5] << 5)) & 0x7F);
            w->coeffs[8 * i + 6] = 64 - (((z[5] >> 2) | ((int32_t)z[6] << 6)) & 0x7F);
            w->coeffs[8 * i + 7] = 64 - (z[6] >> 1);
            z += 7;
        }
    }
    else
    {
        printf("BitUnpack error: b must be in{ 1,2,5,6,7 }!\n");
        exit(0);
    }
}