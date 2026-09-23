/**
 * @file main.c
 * @brief Pure C99 test harness for verifying output of Garnet-512 C-ref implementation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Garnet_512.h"

#define SIZE 32768             /* 32768 = 2^15 */

int main(void)
{
    int i;
    uint8_t* message = NULL;
    uint8_t digest_c[64];
    garnet_u128_t state_c[STATE_4x4];
    uint64_t bitlength = 477;  /* 477 比特 */
    uint64_t byte_num;

    message = (uint8_t*)calloc(SIZE, sizeof(uint8_t));
    if (!message) {
        printf("error\n");
        exit(EXIT_FAILURE);
    }

    if ((bitlength % 8) == 0) {
        byte_num = bitlength / 8;
    } else {
        byte_num = bitlength / 8 + 1;
    }

    
    memset(message, 11, byte_num);
    message[18] = 0x01;

    printf("the original message is\n");
    for (i = 0; i < (int)byte_num; i++) {
        printf("%02x,", message[i]);
        if (((i + 1) % 16) == 0) {
            printf("\n");
        }
    }
    printf("\n");

    pad_message(message, bitlength); /* 填充 */

    printf("after padding is: \n");
    if ((bitlength % 8) != 0) {
        for (i = 0; i < (int)byte_num; i++) {
            printf("%02x,", message[i]);
            if (((i + 1) % 16) == 0) {
                printf("\n");
            }
        }
        printf("\n");
    } else {
        for (i = 0; i < (int)byte_num + 1; i++) {
            printf("%02x,", message[i]);
            if (((i + 1) % 16) == 0) {
                printf("\n");
            }
        }
        printf("\n");
    }

    /* 初始化状态与数据准备（完全采用纯 C 结构体 garnet_u128_t） */
    for (i = 0; i < STATE_4x4; i++) {
        state_c[i].v[0] = 0;
        state_c[i].v[1] = 0;
    }
    memset(digest_c, 0, 64);

    /* 仅调用纯 C99 规范版的 garnet_512，无任何外部硬件加速库依赖 */
    garnet_512(message, bitlength, digest_c, state_c);

    printf("\n-------------------------------------------------------------\n");
    printf("Garnet-512 result: \n");
    for (i = 0; i < 64; i++) {
        printf("%02x", digest_c[i]);
        if (((i + 1) % 8) == 0) {
            printf("\n");
        }
    }
    printf("-------------------------------------------------------------\n");

    free(message);
    return 0;
}