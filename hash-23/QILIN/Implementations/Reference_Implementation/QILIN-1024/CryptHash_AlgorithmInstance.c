/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_AlgorithmInstance.h"

void Qilinf(uint64_t x[7][7],int R){
    const uint64_t rc[16] = {
		0x0000000001027fd7, 0x0000000001027fd6, 0x0000000001027fd5,
		0x0000000001027fd3, 0x0000000001027fc6, 0x0000000001027f95,
		0x0000000001027f53, 0x0000000001027ec6, 0x0000000001027d95,
		0x0000000001027b53, 0x00000000010276c6, 0x0000000001026d95,
        0x0000000001025b53, 0x00000000010236c6, 0x0000000001006d95,
        0x0000000000025b52
	};
    const int n1[28]={5, 2, 4, 1, 6, 3, 0, 7, 10, 11, 14, 8, 16, 12, 9, 13, 11, 8, 18, 19, 20, 24, 22, 25, 23, 21, 26, 27};
    const int n2[28]={2, 4, 1, 6, 3, 0, 7, 5, 12, 9, 13, 15, 17, 18, 19, 20, 21, 22, 23, 24, 24, 22, 25, 23, 21, 26, 27, 20};
    const int m[28]= {0, 0, 0, 0, 0, 0, 1, 39, 23, 18, 46, 54,  5, 15, 26, 33, 46, 44,  9, 30,  0,  0,  0,  0,  0,  0,  1, 39};
    uint64_t y[35][7];
    for(int r=0;r<R;r++){
        //Linear layer
        for(int i=0;i<7;i++){
            for(int j=0;j<7;j++){
                y[i][j]=x[i][j];
            }
        }
        for(int i=0;i<7;i++){
            for(int j=7;j<35;j++){
                y[j][i]=y[n1[j-7]][i]^ROL64(y[n2[j-7]][i],m[j-7]);
            }
        }
        for(int i=0;i<7;i++){
            for(int j=0;j<7;j++){
                y[i][j]=y[i+28][(7-i+j)%7];
            }
        }

        //S-box
        uint64_t t[7][7];
        for(int i=0;i<7;i++){
            t[i][0]=(y[i][1]&y[i][2])^y[i][0];
            t[i][1]=(y[i][2]|y[i][3])^y[i][1];
            t[i][2]=(y[i][3]&y[i][4])^y[i][2];
            t[i][3]=(y[i][4]|y[i][5])^y[i][3];
            t[i][4]=(y[i][5]&y[i][6])^y[i][4];
            t[i][5]=((~y[i][6])&y[i][0])^y[i][5];
            t[i][6]=(y[i][0]|y[i][1])^y[i][6];
            x[i][0]=(t[i][0]&t[i][5])^t[i][2];
            x[(i+1)%7][1]=(t[i][5]|t[i][3])^t[i][0];
            x[(i+2)%7][2]=(t[i][3]&t[i][1])^t[i][5];
            x[(i+3)%7][3]=(t[i][1]|t[i][6])^t[i][3];
            x[(i+4)%7][4]=(t[i][6]&t[i][4])^t[i][1];
            x[(i+5)%7][5]=((~t[i][4])&t[i][2])^t[i][6];
            x[(i+6)%7][6]=(t[i][2]|t[i][0])^t[i][4];
        }
        x[0][0]^=rc[r];
    }
}

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{

    int rate_bits = 3136 - 2 * digest_len_bits;
    int rate_bytes = rate_bits / 8;
    int rate_words = rate_bits / 64;

    int R=12;
    if(digest_len_bits==1024){
        R=16;
    }else if(digest_len_bits==768){
        R=14;
    }

    uint64_t x[7][7] = {0};
    unsigned long long processed_bits = 0;

    // 1. 吸收所有完全由消息构成的完整块
    while (processed_bits + rate_bits <= msg_len_bits) {
        const uint8_t* current_msg_ptr = msg + (processed_bits / 8);
        for (int j = 0; j < rate_words; j++) {
            uint64_t val;
            memcpy(&val, current_msg_ptr + j * 8, 8);
            x[j % 7][j / 7] ^= val;
        }
        Qilinf(x,R);
        processed_bits += rate_bits;
    }

    // 2. 处理最后的消息碎片并添加填充
    uint8_t last_block[265] = {0}; 
    unsigned long long remaining_bits = msg_len_bits - processed_bits;
    unsigned long long remaining_bytes = (remaining_bits + 7) / 8;
    
    // 拷贝剩余字节
    if (remaining_bytes > 0) {
        memcpy(last_block, msg + (processed_bits / 8), remaining_bytes);
    }

    // 清理最后一个字节中不属于消息的位（如果有 partial bits）
    if (remaining_bits % 8 != 0) {
        last_block[remaining_bits / 8] &= (0xFF << (8 - (remaining_bits % 8)));
    }

    // 添加第一个填充位 '1'
    last_block[remaining_bits / 8] |= (1 << (7 - (remaining_bits % 8)));

    // 检查第一个填充位是否正好填满了最后一个字节并达到了 rate 的末尾
    // 如果填充 '1' 后正好达到 rate_bits，需要处理当前块并补一个全 0 块
    if (remaining_bits + 1 == rate_bits) {
        // 当前块已满（包含起始 '1'）
        for (int j = 0; j < rate_words; j++) {
            uint64_t val; memcpy(&val, last_block + j * 8, 8);
            x[j % 7][j / 7] ^= val;
        }
        Qilinf(x,R);
        // 准备一个新的空块用于放置结束填充位 '1'
        memset(last_block, 0, rate_bytes);
    }

    // 在当前（或新的）块的最后一位添加结束填充 '1'
    last_block[rate_bytes - 1] |= 0x01;

    // 吸收最终包含填充的块
    for (int j = 0; j < rate_words; j++) {
        uint64_t val;
        memcpy(&val, last_block + j * 8, 8);
        x[j % 7][j / 7] ^= val;
    }
    Qilinf(x,R);

    // 3. 输出阶段
    for (int i = 0; i < digest_len_bits / 64; i++) {
        uint64_t out_val = x[i % 7][i / 7];
        memcpy(digest + i * 8, &out_val, 8);
    }
    return 0;
}