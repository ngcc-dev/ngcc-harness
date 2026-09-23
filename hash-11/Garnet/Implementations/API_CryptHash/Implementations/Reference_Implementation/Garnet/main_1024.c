#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "Garnet_1024.h"

typedef struct {
    uint64_t v[2];  
} garnet_u128_t;

#define STATE_4x4 16
#define ITERATIONS 100000  // 跑 10 万次避免误差



#define SIZE 32768             /* 32768 = 2^15 */

int main(void)
{
    int i;
    uint8_t* message = NULL;
    uint8_t digest_c[128];      /* Garnet_1024 的摘要长度为 1024 比特 (128 字节) */
    garnet_u128_t state_c[STATE_4x4];
    uint64_t bitlength = 477;   /* 477 比特测试数据 */
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
    message[19] = 0x01;

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
    memset(digest_c, 0, 128);

    /* 调用适配后的 Garnet_1024 算法 */
    Garnet_1024(message, bitlength, digest_c, state_c);

    printf("\n-------------------------------------------------------------\n");
    printf("Garnet-1024 result: \n");
    for (i = 0; i < 128; i++) {
        printf("%02x", digest_c[i]);
        if (((i + 1) % 8) == 0) {
            printf("\n");
        }
    }
    printf("-------------------------------------------------------------\n");

    free(message);
    return 0;
}