/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/
//#include <cstdlib> 
//#include <cstdio>
//#include <assert.h> 
//#include <cstring> 
//#include <vector>
#include <stdio.h>   // ÊäÈëÊä³ö
#include <stdlib.h>  // ÄÚ´æ¡¢¹¤¾ß
#include <string.h>  // ×Ö·û´®
#include <assert.h>  // ¶ÏÑÔ


#include "CryptHash_AlgorithmInstance.h"




#define BlockSize 32//2*HalfBlockSize
#define HalfBlockSize 16// The length of one brunch
#define WordSize 4
#define Step 13
#define ROUNDS 52
//#define B 2//The number of the brunch in compression function
//#define A 3


unsigned char S[256] = {
    0x33, 0x30, 0x36, 0x32, 0x35, 0x34, 0x3f, 0x3e, 0x3a, 0x38, 0x37, 0x39, 0x31, 0x3c, 0x3d, 0x3b,
    0x03, 0x00, 0x06, 0x02, 0x05, 0x04, 0x0f, 0x0e, 0x0a, 0x08, 0x07, 0x09, 0x01, 0x0c, 0x0d, 0x0b,
    0x63, 0x60, 0x66, 0x62, 0x65, 0x64, 0x6f, 0x6e, 0x6a, 0x68, 0x67, 0x69, 0x61, 0x6c, 0x6d, 0x6b,
    0x23, 0x20, 0x26, 0x22, 0x25, 0x24, 0x2f, 0x2e, 0x2a, 0x28, 0x27, 0x29, 0x21, 0x2c, 0x2d, 0x2b,
    0x53, 0x50, 0x56, 0x52, 0x55, 0x54, 0x5f, 0x5e, 0x5a, 0x58, 0x57, 0x59, 0x51, 0x5c, 0x5d, 0x5b,
    0x43, 0x40, 0x46, 0x42, 0x45, 0x44, 0x4f, 0x4e, 0x4a, 0x48, 0x47, 0x49, 0x41, 0x4c, 0x4d, 0x4b,
    0xf3, 0xf0, 0xf6, 0xf2, 0xf5, 0xf4, 0xff, 0xfe, 0xfa, 0xf8, 0xf7, 0xf9, 0xf1, 0xfc, 0xfd, 0xfb,
    0xe3, 0xe0, 0xe6, 0xe2, 0xe5, 0xe4, 0xef, 0xee, 0xea, 0xe8, 0xe7, 0xe9, 0xe1, 0xec, 0xed, 0xeb,
    0xa3, 0xa0, 0xa6, 0xa2, 0xa5, 0xa4, 0xaf, 0xae, 0xaa, 0xa8, 0xa7, 0xa9, 0xa1, 0xac, 0xad, 0xab,
    0x83, 0x80, 0x86, 0x82, 0x85, 0x84, 0x8f, 0x8e, 0x8a, 0x88, 0x87, 0x89, 0x81, 0x8c, 0x8d, 0x8b,
    0x73, 0x70, 0x76, 0x72, 0x75, 0x74, 0x7f, 0x7e, 0x7a, 0x78, 0x77, 0x79, 0x71, 0x7c, 0x7d, 0x7b,
    0x93, 0x90, 0x96, 0x92, 0x95, 0x94, 0x9f, 0x9e, 0x9a, 0x98, 0x97, 0x99, 0x91, 0x9c, 0x9d, 0x9b,
    0x13, 0x10, 0x16, 0x12, 0x15, 0x14, 0x1f, 0x1e, 0x1a, 0x18, 0x17, 0x19, 0x11, 0x1c, 0x1d, 0x1b,
    0xc3, 0xc0, 0xc6, 0xc2, 0xc5, 0xc4, 0xcf, 0xce, 0xca, 0xc8, 0xc7, 0xc9, 0xc1, 0xcc, 0xcd, 0xcb,
    0xd3, 0xd0, 0xd6, 0xd2, 0xd5, 0xd4, 0xdf, 0xde, 0xda, 0xd8, 0xd7, 0xd9, 0xd1, 0xdc, 0xdd, 0xdb,
    0xb3, 0xb0, 0xb6, 0xb2, 0xb5, 0xb4, 0xbf, 0xbe, 0xba, 0xb8, 0xb7, 0xb9, 0xb1, 0xbc, 0xbd, 0xbb };

unsigned char PL[32] = { 28,18,16,26,30,22,29,23,27,17,24,31,25,19,21,20,3,10,4,15,14,5,7,13,12,0,11,1,9,2,6,8 };
unsigned char PR[32] = { 0,2,1,12,11,8,13,7,10,4,6,5,3,14,9,15,22,18,27,23,24,31,20,28,29,17,16,19,25,30,26,21 };
unsigned char F[32] = { 11,3,14,2,9,10,12,0,4,13,7,8,15,6,5,1,16,19,27,22,31,26,20,30,23,29,25,18,24,21,28,17 };
unsigned char G[32] = { 23,3,31,2,0,20,21,15,13,29,14,30,12,28,22,1,17,24,8,5,27,16,6,11,25,18,19,10,7,9,26,4 };


unsigned char RC[Step][16] = { 00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,
0x98,0x8C,0xC9,0xDD,00,00,00,00,00,00,00,00,00,00,00,00,
0xF0,0xE4,0xA1,0xB5,00,00,00,00,00,00,00,00,00,00,00,00,
0x21,0x35,0x70,0x64,00,00,00,00,00,00,00,00,00,00,00,00,
0x83,0x97,0xD2,0xC6,00,00,00,00,00,00,00,00,00,00,00,00,
0xC7,0xD3,0x96,0x82,00,00,00,00,00,00,00,00,00,00,00,00,
0x4F,0x5B,0x1E,0x0A,00,00,00,00,00,00,00,00,00,00,00,00,
0x5E,0x4A,0x0F,0x1B,00,00,00,00,00,00,00,00,00,00,00,00,
0x7C,0x68,0x2D,0x39,00,00,00,00,00,00,00,00,00,00,00,00,
0x39,0x2D,0x68,0x7C,00,00,00,00,00,00,00,00,00,00,00,00,
0xB3,0xA7,0xE2,0xF6,00,00,00,00,00,00,00,00,00,00,00,00,
0xA7,0xB3,0xF6,0xE2,00,00,00,00,00,00,00,00,00,00,00,00,
0x8E,0x9A,0xDF,0xCB,00,00,00,00,00,00,00,00,00,00,00,00 };


unsigned char IV512_1[32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0 };
unsigned char IV512_2[32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0 };

unsigned char IV768_1[32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0 };
unsigned char IV768_2[32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0 };
unsigned char IV768_3[32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

unsigned char IV1024_1[32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0 };
unsigned char IV1024_2[32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0 };
unsigned char IV1024_3[32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
unsigned char IV1024_4[32] = { 0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };



void ROL32_4(unsigned char* x, unsigned char* y)
{
    //The operation <<<_32 4 in round function
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            y[4 * i + j] = (x[4 * i + j] << 4) | (x[4 * i + (j + 1) % 4] >> 4);
}

void ROL32_8(unsigned char* x, unsigned char* y)
{
    //The operation <<<_32 4 in round function
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            y[4 * i + j] = x[4 * i + (j + 1) % 4];
}

void ROL32_20(unsigned char* x, unsigned char* y)
{
    //The operation <<<_32 4 in round function
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            y[4 * i + j] = (x[4 * i + (j + 2) % 4] << 4) | (x[4 * i + (j + 3) % 4] >> 4);
}

void NibblePermuitation(unsigned char* x, unsigned char* y, unsigned char* P)
{
    int i;
    unsigned char AA[2 * HalfBlockSize] = { 0 };
    unsigned char BB[2 * HalfBlockSize] = { 0 };
    //Extend the x to A len(A)=2len(x), A[2*i]=x[i]>>4, A[2*i+1]=x[i]&0xf
    for (i = 0; i < HalfBlockSize; i++)
    {
        AA[2 * i] = x[i] >> 4;
        AA[2 * i + 1] = x[i] & 0xf;
    }
    //Perform P permutation
    for (i = 0; i < BlockSize; i++)
        BB[i] = AA[P[i]];
    //Transform BB to y
    for (i = 0; i < HalfBlockSize; i++)
        y[i] = BB[2 * i] << 4 | BB[2 * i + 1];


}
void XOR(unsigned char* x, unsigned char* y, unsigned char* z)
{
    int i;
    for (i = 0; i < HalfBlockSize; i++)
        z[i] = x[i] ^ y[i];
}

void RoundFunction(unsigned char* P, unsigned char* C, unsigned char* RK)
{
    /*
    * This is one round of the Block Cipher
    * Input:unsigned char P/C/RK[32]={0x??,0x??,...,0x??}, P=X|Y;
    */
    int i;
    unsigned char X[HalfBlockSize] = { 0 };
    unsigned char Y[HalfBlockSize] = { 0 };
    unsigned char R[HalfBlockSize] = { 0 };
    unsigned char CL[HalfBlockSize] = { 0 };
    unsigned char CR[HalfBlockSize] = { 0 };
    unsigned char PK[BlockSize] = { 0 };

    //unsigned char SX[HalfBlockSize] = { 0 };
    //unsigned char SY[HalfBlockSize] = { 0 };
    //Add Round Key
    for (i = 0; i < BlockSize; i++)
        PK[i] = P[i] ^ RK[i];

    //Divide the P to X||Y
    for (i = 0; i < HalfBlockSize; i++)
    {
        X[i] = PK[i];
        Y[i] = PK[i + HalfBlockSize];

    }
    //The S-Box Layer
    for (i = 0; i < HalfBlockSize; i++)
    {
        X[i] = S[X[i]];
        Y[i] = S[Y[i]];
    }

    //printf("-------------------------------------------------------\n");
    //printf("X= ");
    //for (int ttt = 0; ttt < HalfBlockSize; ttt++)
    //    printf("%02x ", X[ttt]);
    //printf("\n");
    //printf("Y= ");
    //for (int ttt = 0; ttt < HalfBlockSize; ttt++)
    //    printf("%02x ", Y[ttt]);
    //printf("\n");


    //The Linear Layer
    for (i = 0; i < HalfBlockSize; i++)
        Y[i] = X[i] ^ Y[i];
    //printf("-------------------------------------------------------\n");

    //printf("Y= ");
    //for (int ttt = 0; ttt < HalfBlockSize; ttt++)
    //    printf("%02x ", Y[ttt]);
    //printf("\n");
    ROL32_4(Y, R);
    for (i = 0; i < HalfBlockSize; i++)
        X[i] = X[i] ^ R[i];
    //printf("-------------------------------------------------------\n");

    //printf("X= ");
    //for (int ttt = 0; ttt < HalfBlockSize; ttt++)
    //    printf("%02x ", X[ttt]);
    //printf("\n");
    ROL32_8(X, R);
    for (i = 0; i < HalfBlockSize; i++)
        Y[i] = Y[i] ^ R[i];
    //printf("-------------------------------------------------------\n");

    //printf("Y= ");
    //for (int ttt = 0; ttt < HalfBlockSize; ttt++)
     //   printf("%02x ", Y[ttt]);
    //printf("\n");
    ROL32_8(Y, R);
    for (i = 0; i < HalfBlockSize; i++)
        X[i] = X[i] ^ R[i];
    //printf("-------------------------------------------------------\n");

    //printf("X= ");
    //for (int ttt = 0; ttt < HalfBlockSize; ttt++)
       // printf("%02x ", X[ttt]);
    //printf("\n");
    ROL32_20(X, R);
    for (i = 0; i < HalfBlockSize; i++)
        Y[i] = Y[i] ^ R[i];
    //printf("-------------------------------------------------------\n");

    //printf("Y= ");
    //for (int ttt = 0; ttt < HalfBlockSize; ttt++)
    //    printf("%02x ", Y[ttt]);
    //printf("\n");
    for (i = 0; i < HalfBlockSize; i++)
        X[i] = X[i] ^ Y[i];
    //printf("-------------------------------------------------------\n");

    //printf("X= ");
    //for (int ttt = 0; ttt < HalfBlockSize; ttt++)
    //    printf("%02x ", X[ttt]);
    //printf("\n");


    NibblePermuitation(X, CL, PL);
    NibblePermuitation(Y, CR, PR);

    //Combine CL||CR--->C
    for (i = 0; i < HalfBlockSize; i++)
    {
        C[i] = CL[i];
        C[i + HalfBlockSize] = CR[i];

    }


}

void KeySchedule(unsigned char* K, unsigned char** RK)
{
    //K is the 1024=128*8 bit master key,K[128]
    //RK[ROUNDS][32] is the 256-bit Round key in every round.
    //RK needs to be initialized
    unsigned char k[8][16] = { 0 };
    //(*k)[16] = (unsigned char(*)[16])K;
    //unsigned char** RK = (unsigned char**)malloc((ROUNDS + 1) * sizeof(unsigned char*));
    //for (int row = 0; row < ROUNDS + 1; row++)
        //RK[row] = (unsigned char*)malloc(BlockSize);

    unsigned char m[8][16] = { 0 };
    int r, i, s;
    for (r = 0; r < 8; r++)
        for (i = 0; i < HalfBlockSize; i++)
            k[r][i] = K[r * 16 + i];
    for (r = 0; r < 4; r++)
    {
        for (i = 0; i < BlockSize; i++)
        {
            RK[r][i] = K[r * BlockSize + i];
        }
    }
    /*
    printf("The K in Step-0\n");
    for (int kk = 0; kk < 8; kk++)
    {
        for (int yy = 0; yy < HalfBlockSize; yy++)
        {
            printf("%02x ", k[kk][yy]);
        }
        printf("\n");
    }
    */
    //memcpy(m, k, sizeof(m));
    for (r = 0; r < 8; r++)
        for (i = 0; i < HalfBlockSize; i++)
            m[r][i] = k[r][i];
    for (s = 1; s < Step; s++)
    {

        //memcpy(k[0], m[2], sizeof(k[0]));
        //memcpy(k[1], m[5], sizeof(k[1]));
        //memcpy(k[2], m[1], sizeof(k[2]));
        //memcpy(k[3], m[0], sizeof(k[3]));
        //memcpy(k[4], m[3], sizeof(k[4]));
        //memcpy(k[5], m[6], sizeof(k[5]));
        //memcpy(k[6], m[7], sizeof(k[6]));
        //memcpy(k[7], m[4], sizeof(k[7]));
        for (i = 0; i < HalfBlockSize; i++)
        {
            k[0][i] = m[2][i];
            k[1][i] = m[5][i];
            k[2][i] = m[1][i];
            k[3][i] = m[0][i];
            k[4][i] = m[3][i];
            k[5][i] = m[6][i];
            k[6][i] = m[7][i];
            k[7][i] = m[4][i];

        }

        /*
        printf("The K in Step-%d\n", s);
        for (int kk = 0; kk < 8; kk++)
        {
            for (int yy = 0; yy < HalfBlockSize; yy++)
            {
                printf("%02x ", k[kk][yy]);
            }
            printf("\n");
        }
        */

        XOR(k[0], RC[s], k[0]);
        NibblePermuitation(k[0], m[0], F);
        NibblePermuitation(k[1], m[1], G);
        NibblePermuitation(k[2], m[2], F);
        NibblePermuitation(k[3], m[3], G);
        NibblePermuitation(k[4], m[4], F);
        NibblePermuitation(k[5], m[5], G);
        NibblePermuitation(k[6], m[6], F);
        NibblePermuitation(k[7], m[7], G);
        for (r = 0; r < 4; r++)
        {
            for (i = 0; i < HalfBlockSize; i++)
            {
                RK[s * 4 + r][i] = m[2 * r][i];
                RK[s * 4 + r][i + HalfBlockSize] = m[2 * r + 1][i];
            }
        }
    }
    //RK[ROUNDS] = RK[ROUNDS-4];
    for (int j = 0; j < BlockSize; j++)
        RK[ROUNDS][j] = RK[ROUNDS - 4][j];
    //memcpy(RK[ROUNDS], RK[ROUNDS - 4], sizeof(RK[ROUNDS]));


}

void BlockCipher(unsigned char* P, unsigned char* C, unsigned char* K)
{
    //P is the plaintext (256 bits), C is the ciphertext (256 bits), K is the master key (1024 bits)

    //Initialize RK
    unsigned char** RK = (unsigned char**)malloc((ROUNDS + 1) * sizeof(unsigned char*));
    for (int row = 0; row < ROUNDS + 1; row++)
        RK[row] = (unsigned char*)malloc(BlockSize);



    KeySchedule(K, RK);

    /*
    printf("The value of K:::\n");
    for (int t = 0; t < 8 * HalfBlockSize; t++)
        printf("%02x ", K[t]);
    printf("\n");


    for (int rr = 0; rr < ROUNDS + 1; rr++)
    {
        printf("The value of the %d-th Round RK:::\n",rr);
        for (int tt = 0; tt < BlockSize; tt++)
            printf("%02x ", RK[rr][tt]);
        printf("\n");
    }
    */



    for (int r = 0; r < ROUNDS; r++)
    {
        //printf("************The %d-th Round***********\n", r);
        RoundFunction(P, C, RK[r]);
        //printf("\n");
        //printf("The %d-th Round\n", r);
        //printf("The P is:::\n");
        //for (int j = 0; j < BlockSize; j++)
        //    printf("%02x ", P[j]);
        //printf("\n");
        //printf("The RK is:::\n");
        //for (int j = 0; j < BlockSize; j++)
        //    printf("%02x ", RK[r][j]);
        //printf("\n");
        //printf("The C is:::\n");
        //for (int j = 0; j < BlockSize; j++)
        //    printf("%02x ", C[j]);
        //printf("\n");
        //printf("*******************************************************\n");

        for (int j = 0; j < BlockSize; j++)
            P[j] = C[j];


    }

    //*** Modify ***
    for (int i = 0; i < BlockSize; i++)
        C[i] = P[i];

    /*
    printf("The C XOR Final Key is:::\n");
    for (int j = 0; j < BlockSize; j++)
        printf("%02x ", C[j]);
    printf("\n");
    printf("*******************************************************\n");
    */

    // ÊÍ·ÅË³Ðò£ºÏÈÊÍ·ÅÃ¿Ò»ÐÐ£¬ÔÙÊÍ·ÅÖ¸ÕëÊý×é
    if (RK != NULL) {
        // 1. ÊÍ·ÅÃ¿Ò»ÐÐµÄÄÚ´æ
        for (int row = 0; row < ROUNDS + 1; row++) {
            if (RK[row] != NULL) {
                free(RK[row]);
                RK[row] = NULL; // ÊÍ·ÅºóÖÃ¿Õ£¬·ÀÖ¹Ò°Ö¸Õë
            }
        }
        // 2. ÊÍ·ÅÖ¸ÕëÊý×é±¾Éí
        free(RK);
        RK = NULL;
    }


}
void Padding(const unsigned char* msg, unsigned char* M, unsigned long long msg_len_bits, int A)
{
    //msg[msg_len_bits]={0x12,0x34,0x56,0x78,0x9a};
    //***NOTE***
    //The correctness details of padding should be verified.
    unsigned long long msg_len_bytes, msg_len_bytes_col;//msg_len_bytes is xia qu zheng
    unsigned long long AN_bits = A * BlockSize * 8;
    unsigned long long row, col, actual_len_bytes, i, j;
    row = (int)(msg_len_bits / AN_bits) + 1;
    row = AN_bits * row;
    actual_len_bytes = (int)(row / 8);
    msg_len_bytes = (int)(msg_len_bits / 8);
    //col = msg_len_bits % AN_bits;
    msg_len_bytes_col = msg_len_bits % 8;
    //M = (unsigned char*)malloc(actual_len_bytes);
    //memset(M, 0, actual_len_bytes);
    for (i = 0; i < msg_len_bytes; i++)
        M[i] = msg[i];
    if (msg_len_bytes_col == 0)
        M[msg_len_bytes] = 0x80;
    else
        M[msg_len_bytes] = msg[msg_len_bytes] + (1 << (7 - msg_len_bytes_col));


    /*
    printf("The value after Paddint:::\n");
    for (int t = 0; t < actual_len_bytes; t++)
        printf(" % 02x ", M[t]);
    printf("\n");
    */

    return;//The length of message after padding
}

void uHash512(const unsigned char* msg, unsigned long long msg_len_bits, unsigned char* digest)
{
    unsigned char* M;//The message after padding
    unsigned long long actual_len_bytes;
    unsigned long long l, i, j;
    unsigned char* P0;
    unsigned char* C0;
    unsigned char* P1;
    unsigned char* C1;
    unsigned char* U1;
    unsigned char* K;
    unsigned char* K0;
    unsigned long long row, col;
    unsigned char FB[BlockSize] = { 0 };
    int A = 3;
    unsigned long long AN_bits = A * BlockSize * 8;
    row = (int)(msg_len_bits / AN_bits) + 1;
    row = AN_bits * row;
    actual_len_bytes = (int)(row / 8);
    M = (unsigned char*)malloc(actual_len_bytes);
    for (int tt = 0; tt < actual_len_bytes; tt++)
        M[tt] = 0;
    //memset(M, 0, actual_len_bytes);

    P0 = (unsigned char*)malloc(BlockSize);//Equal to u_i^0. i,e, ui<--(P0,P1)
    C0 = (unsigned char*)malloc(BlockSize);//Equal to u__i+1^0
    P1 = (unsigned char*)malloc(BlockSize);//Equal to u_i^1
    C1 = (unsigned char*)malloc(BlockSize);//Equal to u__i+1^1,ui+1<--(C0,C1)
    U1 = (unsigned char*)malloc(BlockSize);//Equal to u__i+1^1,ui+1<--(C0,C1
    K = (unsigned char*)malloc(8 * HalfBlockSize);//Equal to K_i
    K0 = (unsigned char*)malloc(8 * HalfBlockSize);//Equal to K_i



    //Step-1: Padding 
    Padding(msg, M, msg_len_bits, A);
    l = (int)(actual_len_bytes / (3 * BlockSize));//The number of blocks of message M



    //Step-2  Dividing the message in M to l an-bit block and process l-1 steps
    for (j = 0; j < BlockSize; j++)
    {
        P0[j] = IV512_1[j];
        P1[j] = IV512_1[j];
        U1[j] = IV512_2[j];
        FB[j] = P0[j];
    }


    for (i = 0; i < l - 1; i++)
    {
        P1[BlockSize - 1] = P1[BlockSize - 1] ^ 1;
        //Give the value to K
        for (j = 0; j < BlockSize; j++)
        {
            K[j] = U1[j];
            K[j + BlockSize] = M[i * 3 * BlockSize + j];
            K[j + 2 * BlockSize] = M[i * 3 * BlockSize + BlockSize + j];
            K[j + 3 * BlockSize] = M[i * 3 * BlockSize + 2 * BlockSize + j];
        }

        /*
        printf("***The value of K***:::\n");
        for (int t = 0; t < 8 * HalfBlockSize; t++)
            printf("%02x ", K[t]);
        printf("\n");
        */
        //Block Cipher Encryption
        //printf("The first Block\n")
        BlockCipher(P0, C0, K);

        BlockCipher(P1, C1, K);
        //FeedBack
        for (j = 0; j < BlockSize; j++)
        {
            U1[j] = C1[j] ^ FB[j];
            P0[j] = C0[j] ^ FB[j];
            P1[j] = P0[j];

        }
        for (j = 0; j < BlockSize; j++)
            FB[j] = P0[j];
        //Diff P0 and P1
        //P1[BlockSize - 1] = P1[BlockSize - 1] ^ 1;        
    }



    //P0[BlockSize - 1] = P0[BlockSize - 1] ^ 0x0;
    P1[BlockSize - 1] = P1[BlockSize - 1] ^ 1;


    //Give the value to K
    for (j = 0; j < BlockSize; j++)
    {
        K[j] = U1[j];
        K[j + BlockSize] = M[(l - 1) * 3 * BlockSize + j];
        K[j + 2 * BlockSize] = M[(l - 1) * 3 * BlockSize + BlockSize + j];
        K[j + 3 * BlockSize] = M[(l - 1) * 3 * BlockSize + 2 * BlockSize + j];
    }

    K[4 * BlockSize - 1] = K[4 * BlockSize - 1] ^ 0x0f;


    //Block Cipher Encryption
    //printf("####################The First Brunch###################\n");

    BlockCipher(P0, C0, K);
    //printf("####################The Second Brunch###################\n");
    K[4 * BlockSize - 1] = K[4 * BlockSize - 1] ^ 0x1f;
    BlockCipher(P1, C1, K);
    //FeedBack
    for (j = 0; j < BlockSize; j++)
    {
        digest[j] = C0[j] ^ FB[j];
        digest[j + BlockSize] = C1[j] ^ FB[j];
    }

    /*
    printf("The hash value after uHash512 is:\n");
    for (j = 0; j < 2 * BlockSize; j++)
        printf("%x", digest[j]);
    printf("\n");
    */


    if (M != NULL) {
        free(M);   // ÊÍ·ÅÄÚ´æ
        M = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (P0 != NULL) {
        free(P0);   // ÊÍ·ÅÄÚ´æ
        P0 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (P1 != NULL) {
        free(P1);   // ÊÍ·ÅÄÚ´æ
        P1 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (C0 != NULL) {
        free(C0);   // ÊÍ·ÅÄÚ´æ
        C0 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (C1 != NULL) {
        free(C1);   // ÊÍ·ÅÄÚ´æ
        C1 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (U1 != NULL) {
        free(U1);   // ÊÍ·ÅÄÚ´æ
        U1 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (K != NULL) {
        free(K);   // ÊÍ·ÅÄÚ´æ
        K = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    return;
}

void uHash768(const unsigned char* msg, unsigned long long msg_len_bits, unsigned char* digest)
{
    unsigned char* M;//The message after padding
    unsigned long long actual_len_bytes;
    unsigned long long l, i, j;
    unsigned char* P0;
    unsigned char* C0;
    unsigned char* P1;
    unsigned char* C1;
    unsigned char* P2;
    unsigned char* C2;
    unsigned char* U1;
    unsigned char* U2;
    unsigned char* K;
    unsigned char FB[BlockSize] = { 0 };
    unsigned long long row, col;
    int A = 2;
    unsigned long long AN_bits = A * BlockSize * 8;
    row = ((int)(msg_len_bits / AN_bits)) + 1;
    row = AN_bits * row;
    actual_len_bytes = (int)(row / 8);
    M = (unsigned char*)malloc(actual_len_bytes);
    for (int tt = 0; tt < actual_len_bytes; tt++)
        M[tt] = 0;
    //memset(M, 0, actual_len_bytes);

    P0 = (unsigned char*)malloc(BlockSize);//Equal to u_i^0. i,e, ui<--(P0,P1)
    C0 = (unsigned char*)malloc(BlockSize);//Equal to u__i+1^0
    P1 = (unsigned char*)malloc(BlockSize);//Equal to u_i^1
    C1 = (unsigned char*)malloc(BlockSize);//Equal to u__i+1^1,ui+1<--(C0,C1)
    P2 = (unsigned char*)malloc(BlockSize);//Equal to u_i^0. i,e, ui<--(P0,P1)
    C2 = (unsigned char*)malloc(BlockSize);//Equal to u__i+1^0
    U1 = (unsigned char*)malloc(BlockSize);//Equal to u__i+1^1,ui+1<--(C0,C1
    U2 = (unsigned char*)malloc(BlockSize);//Equal to u__i+1^1,ui+1<--(C0,C1
    K = (unsigned char*)malloc(8 * HalfBlockSize);//Equal to K_i

    //M = (unsigned char*)malloc(actual_len_bytes);
    //memset(M, 0, actual_len_bytes);

    //Step-1: Padding 
    Padding(msg, M, msg_len_bits, A);
    l = (int)(actual_len_bytes / (2 * BlockSize));//The number of blocks of message M

    //printf("The padding is:::\n");
    //for (int tt = 0; tt < actual_len_bytes; tt++)
    //    printf("%02x ", M[tt]);
    //printf("\n");


    //Step-2  Dividing the message in M to l an-bit block and process l-1 steps
    for (j = 0; j < BlockSize; j++)
    {
        P0[j] = IV768_1[j];
        P1[j] = IV768_1[j];
        P2[j] = IV768_1[j];
        U1[j] = IV768_2[j];
        U2[j] = IV768_3[j];
        FB[j] = P0[j];
    }

    for (i = 0; i < l - 1; i++)
    {
        P1[BlockSize - 1] = P1[BlockSize - 1] ^ 1;
        P2[BlockSize - 1] = P2[BlockSize - 1] ^ 2;
        //Give the value to K
        for (j = 0; j < BlockSize; j++)
        {
            K[j] = U1[j];
            K[j + BlockSize] = U2[j];
            K[j + 2 * BlockSize] = M[i * 2 * BlockSize + j];
            K[j + 3 * BlockSize] = M[i * 2 * BlockSize + BlockSize + j];
        }
        //Block Cipher Encryption
        BlockCipher(P0, C0, K);
        BlockCipher(P1, C1, K);
        BlockCipher(P2, C2, K);
        //FeedBack
        for (j = 0; j < BlockSize; j++)
        {
            U1[j] = C1[j] ^ FB[j];
            U2[j] = C2[j] ^ FB[j];
            P0[j] = C0[j] ^ FB[j];
            P1[j] = P0[j];
            P2[j] = P0[j];
        }
        for (j = 0; j < BlockSize; j++)
            FB[j] = P0[j];

    }
    P0[BlockSize - 1] = P0[BlockSize - 1] ^ 0x0;
    P1[BlockSize - 1] = P1[BlockSize - 1] ^ 0x1;
    P2[BlockSize - 1] = P2[BlockSize - 1] ^ 0x2;
    //Give the value to K
    for (j = 0; j < BlockSize; j++)
    {
        K[j] = U1[j];
        K[j + BlockSize] = U2[j];
        K[j + 2 * BlockSize] = M[(l - 1) * 2 * BlockSize + j];
        K[j + 3 * BlockSize] = M[(l - 1) * 2 * BlockSize + BlockSize + j];
    }
    //Block Cipher Encryption
    //printf("The P0:::\n");
    //for (int tt = 0; tt < BlockSize; tt++)
    //    printf("%02x ", P0[tt]);
    //printf("\n");

    //printf("The K:::\n");
    //for (int tt = 0; tt < 4 * BlockSize; tt++)
    //{
    //    if (tt % BlockSize == 0)
    //        printf("\n");
    //    printf("%02x ", K[tt]);

    //}
    //printf("\n");
    K[4 * BlockSize - 1] = K[4 * BlockSize - 1] ^ 0xf;
    BlockCipher(P0, C0, K);

    //printf("The FB:::\n");
    //for (int tt = 0; tt < BlockSize; tt++)
    //    printf("%02x ", FB[tt]);
    //printf("\n");

    //printf("The first Brunch Output:::\n");
    //for (int tt = 0; tt < BlockSize; tt++)
    //    printf("%02x ", C0[tt]);
    //printf("\n");
    K[4 * BlockSize - 1] = K[4 * BlockSize - 1] ^ 0x1f;
    BlockCipher(P1, C1, K);
    K[4 * BlockSize - 1] = K[4 * BlockSize - 1] ^ 0x1;
    BlockCipher(P2, C2, K);
    //FeedBack
    for (j = 0; j < BlockSize; j++)
    {
        digest[j] = C0[j] ^ FB[j];
        digest[j + BlockSize] = C1[j] ^ FB[j];
        digest[j + 2 * BlockSize] = C2[j] ^ FB[j];
    }

    //printf("The hash value after uHash768 is:\n");
    //for (j = 0; j < 3 * BlockSize; j++)
    //    printf("%x", digest[j]);
   //printf("\n");

    if (M != NULL) {
        free(M);   // ÊÍ·ÅÄÚ´æ
        M = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (P0 != NULL) {
        free(P0);   // ÊÍ·ÅÄÚ´æ
        P0 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (P1 != NULL) {
        free(P1);   // ÊÍ·ÅÄÚ´æ
        P1 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (P2 != NULL) {
        free(P2);   // ÊÍ·ÅÄÚ´æ
        P2 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (C0 != NULL) {
        free(C0);   // ÊÍ·ÅÄÚ´æ
        C0 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (C1 != NULL) {
        free(C1);   // ÊÍ·ÅÄÚ´æ
        C1 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (C2 != NULL) {
        free(C2);   // ÊÍ·ÅÄÚ´æ
        C2 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (U1 != NULL) {
        free(U1);   // ÊÍ·ÅÄÚ´æ
        U1 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (U2 != NULL) {
        free(U2);   // ÊÍ·ÅÄÚ´æ
        U2 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (K != NULL) {
        free(K);   // ÊÍ·ÅÄÚ´æ
        K = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    return;
}

void uHash1024(const unsigned char* msg, unsigned long long msg_len_bits, unsigned char* digest)
{
    unsigned char* M;//The message after padding
    unsigned long long actual_len_bytes;
    unsigned long long l, i, j;
    unsigned char* P0;
    unsigned char* C0;
    unsigned char* P1;
    unsigned char* C1;
    unsigned char* P2;
    unsigned char* C2;
    unsigned char* P3;
    unsigned char* C3;
    unsigned char* U1;
    unsigned char* U2;
    unsigned char* U3;
    unsigned char* K;
    unsigned char* K0;
    unsigned char* K1;
    unsigned char* K2;


    unsigned char FB[BlockSize] = { 0 };
    unsigned long long row, col;
    int A = 1;
    unsigned long long AN_bits = A * BlockSize * 8;
    row = (int)(msg_len_bits / AN_bits) + 1;
    row = AN_bits * row;
    actual_len_bytes = (int)(row / 8);
    M = (unsigned char*)malloc(actual_len_bytes);
    for (int tt = 0; tt < actual_len_bytes; tt++)
        M[tt] = 0;
    //memset(M, 0, actual_len_bytes);

    P0 = (unsigned char*)malloc(BlockSize);//Equal to u_i^0. i,e, ui<--(P0,P1)
    C0 = (unsigned char*)malloc(BlockSize);//Equal to u__i+1^0
    P1 = (unsigned char*)malloc(BlockSize);//Equal to u_i^1
    C1 = (unsigned char*)malloc(BlockSize);//Equal to u__i+1^1,ui+1<--(C0,C1)
    P2 = (unsigned char*)malloc(BlockSize);//Equal to u_i^0. i,e, ui<--(P0,P1)
    C2 = (unsigned char*)malloc(BlockSize);//Equal to u__i+1^0
    P3 = (unsigned char*)malloc(BlockSize);//Equal to u_i^0. i,e, ui<--(P0,P1)
    C3 = (unsigned char*)malloc(BlockSize);//Equal to u__i+1^0
    U1 = (unsigned char*)malloc(BlockSize);//Equal to u__i+1^1,ui+1<--(C0,C1
    U2 = (unsigned char*)malloc(BlockSize);//Equal to u__i+1^1,ui+1<--(C0,C1
    U3 = (unsigned char*)malloc(BlockSize);//Equal to u__i+1^1,ui+1<--(C0,C1
    K = (unsigned char*)malloc(8 * HalfBlockSize);//Equal to K_i
    K0 = (unsigned char*)malloc(8 * HalfBlockSize);//Equal to K_i
    K1 = (unsigned char*)malloc(8 * HalfBlockSize);//Equal to K_i
    K2 = (unsigned char*)malloc(8 * HalfBlockSize);//Equal to K_i


    //Step-1: Padding 
    Padding(msg, M, msg_len_bits, A);
    l = (int)(actual_len_bytes / BlockSize);//The number of blocks of message M

    //M = (unsigned char*)malloc(actual_len_bytes);
    //memset(M, 0, actual_len_bytes);

    //Step-2  Dividing the message in M to l an-bit block and process l-1 steps
    for (j = 0; j < BlockSize; j++)
    {
        P0[j] = IV1024_1[j];
        P1[j] = IV1024_1[j];
        P2[j] = IV1024_1[j];
        P3[j] = IV1024_1[j];
        U1[j] = IV1024_2[j];
        U2[j] = IV1024_3[j];
        U3[j] = IV1024_4[j];
        FB[j] = P0[j];
    }

    for (i = 0; i < l - 1; i++)
    {
        P1[BlockSize - 1] = P1[BlockSize - 1] ^ 1;
        P2[BlockSize - 1] = P2[BlockSize - 1] ^ 2;
        P3[BlockSize - 1] = P3[BlockSize - 1] ^ 3;
        //Give the value to K
        for (j = 0; j < BlockSize; j++)
        {
            K[j] = U1[j];
            K[j + BlockSize] = U2[j];
            K[j + 2 * BlockSize] = U3[j];
            K[j + 3 * BlockSize] = M[i * BlockSize + j];
        }
        //Block Cipher Encryption
        BlockCipher(P0, C0, K);
        BlockCipher(P1, C1, K);
        BlockCipher(P2, C2, K);
        BlockCipher(P3, C3, K);
        //FeedBack
        for (j = 0; j < BlockSize; j++)
        {
            U1[j] = C1[j] ^ FB[j];
            U2[j] = C2[j] ^ FB[j];
            U3[j] = C3[j] ^ FB[j];
            P0[j] = C0[j] ^ FB[j];
            P1[j] = P0[j];
            P2[j] = P0[j];
            P3[j] = P0[j];
        }
        for (j = 0; j < BlockSize; j++)
            FB[j] = P0[j];

    }
    P0[BlockSize - 1] = P0[BlockSize - 1] ^ 0x0;
    P1[BlockSize - 1] = P1[BlockSize - 1] ^ 0x1;
    P2[BlockSize - 1] = P2[BlockSize - 1] ^ 0x2;
    P3[BlockSize - 1] = P3[BlockSize - 1] ^ 0x3;
    //Give the value to K
    for (j = 0; j < BlockSize; j++)
    {
        K[j] = U1[j];
        K[j + BlockSize] = U2[j];
        K[j + 2 * BlockSize] = U3[j];
        K[j + 3 * BlockSize] = M[(l - 1) * BlockSize + j];
    }

    for (j = 0; j < BlockSize; j++)
    {
        K0[j] = U1[j];
        K0[j + BlockSize] = U2[j];
        K0[j + 2 * BlockSize] = U3[j];
        K0[j + 3 * BlockSize] = M[(l - 1) * BlockSize + j];
    }

    for (j = 0; j < BlockSize; j++)
    {
        K1[j] = U1[j];
        K1[j + BlockSize] = U2[j];
        K1[j + 2 * BlockSize] = U3[j];
        K1[j + 3 * BlockSize] = M[(l - 1) * BlockSize + j];
    }

    for (j = 0; j < BlockSize; j++)
    {
        K2[j] = U1[j];
        K2[j + BlockSize] = U2[j];
        K2[j + 2 * BlockSize] = U3[j];
        K2[j + 3 * BlockSize] = M[(l - 1) * BlockSize + j];
    }

    K[4 * BlockSize - 1] = K[4 * BlockSize - 1] ^ 0xf;
    //Block Cipher Encryption
    BlockCipher(P0, C0, K);
    K0[4 * BlockSize - 1] = K0[4 * BlockSize - 1] ^ 0x10;
    BlockCipher(P1, C1, K0);
    K1[4 * BlockSize - 1] = K1[4 * BlockSize - 1] ^ 0x11;
    BlockCipher(P2, C2, K1);
    K2[4 * BlockSize - 1] = K2[4 * BlockSize - 1] ^ 0x12;
    BlockCipher(P3, C3, K2);
    //FeedBack
    for (j = 0; j < BlockSize; j++)
    {
        digest[j] = C0[j] ^ FB[j];
        digest[j + BlockSize] = C1[j] ^ FB[j];
        digest[j + 2 * BlockSize] = C2[j] ^ FB[j];
        digest[j + 3 * BlockSize] = C3[j] ^ FB[j];
    }
    //printf("The hash value after uHash1024 is:\n");
    //for (j = 0; j < 4 * BlockSize; j++)
    //    printf("%x", digest[j]);
    //printf("\n");
    if (M != NULL) {
        free(M);   // ÊÍ·ÅÄÚ´æ
        M = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (P0 != NULL) {
        free(P0);   // ÊÍ·ÅÄÚ´æ
        P0 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (P1 != NULL) {
        free(P1);   // ÊÍ·ÅÄÚ´æ
        P1 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (P2 != NULL) {
        free(P2);   // ÊÍ·ÅÄÚ´æ
        P2 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (P3 != NULL) {
        free(P3);   // ÊÍ·ÅÄÚ´æ
        P3 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (C0 != NULL) {
        free(C0);   // ÊÍ·ÅÄÚ´æ
        C0 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (C1 != NULL) {
        free(C1);   // ÊÍ·ÅÄÚ´æ
        C1 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (C2 != NULL) {
        free(C2);   // ÊÍ·ÅÄÚ´æ
        C2 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (C3 != NULL) {
        free(C3);   // ÊÍ·ÅÄÚ´æ
        C3 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (U1 != NULL) {
        free(U1);   // ÊÍ·ÅÄÚ´æ
        U1 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (U2 != NULL) {
        free(U2);   // ÊÍ·ÅÄÚ´æ
        U2 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (U3 != NULL) {
        free(U3);   // ÊÍ·ÅÄÚ´æ
        U3 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }
    if (K != NULL) {
        free(K);   // ÊÍ·ÅÄÚ´æ
        K = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }

    if (K0 != NULL) {
        free(K0);   // ÊÍ·ÅÄÚ´æ
        K0 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }

    if (K1 != NULL) {
        free(K1);   // ÊÍ·ÅÄÚ´æ
        K1 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }

    if (K2 != NULL) {
        free(K2);   // ÊÍ·ÅÄÚ´æ
        K2 = NULL;  // ÖÃ¿Õ£¬±ÜÃâÒ°Ö¸Õë
    }


    return;
}



int CryptHash(int digest_len_bits, const unsigned char* msg, unsigned long long msg_len_bits, unsigned char* digest)
{

    if (digest_len_bits == 512)
        uHash512(msg, msg_len_bits, digest);
    if (digest_len_bits == 768)
        uHash768(msg, msg_len_bits, digest);
    if (digest_len_bits == 1024)
        uHash1024(msg, msg_len_bits, digest);
    return 0;
}


//int main()
//{
//    unsigned char* msg;
//
//
//    unsigned char* digest;
//    //unsigned char* RK;
//    unsigned long long msg_len_bits = 1024;
//    unsigned long long dig_len = 1024;
//
//    msg = (unsigned char*)malloc((int)msg_len_bits / 8 + 1);
//    digest = (unsigned char*)malloc((int)dig_len / 8);
//
//    //RK = (unsigned char*)malloc(BlockSize);
//
//    for (int t = 0; t < (int)msg_len_bits / 8 + 1; t++)
//    {
//        msg[t] = 0;
//    }
//    msg[0] = 0x30;
//
//    //memset(P, 0, BlockSize);
//    //memset(C, 0, BlockSize);
//    CryptHash(dig_len, msg, msg_len_bits, digest);
//    printf("Plaintext:::\n");
//    for (int i = 0; i < (int)msg_len_bits / 8 + 1; i++)
//        printf("%02x ", msg[i]);
//    printf("\n");
//
//    printf("Digest:::\n");
//    for (int i = 0; i < (int)dig_len / 8; i++)
//        printf("%02x ", digest[i]);
//    printf("\n");
//
//
//
//
//
//
//}



