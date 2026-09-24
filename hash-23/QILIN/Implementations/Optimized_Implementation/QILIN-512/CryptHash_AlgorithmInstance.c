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
#include "QilinF.h"

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
    int rate_bits = 3136 - 2 * digest_len_bits;
    int rate_bytes = rate_bits / 8;

    int R=12;
    if(digest_len_bits==1024){
        R=16;
    }else if(digest_len_bits==768){
        R=14;
    }

    
    unsigned long long processed_bits = 0;
    unsigned long long processed_block = 0;

    declare;

    // 1. 吸收所有完全由消息构成的完整块
    while (processed_bits + rate_bits <= msg_len_bits) {

        if(digest_len_bits==512){
            absorb512(msg,processed_block);
        }else if(digest_len_bits==1024){
            absorb1024(msg,processed_block);
        }else if(digest_len_bits==768){
            absorb768(msg,processed_block);
        }else{
            return -1;
        }
        for(int i=0;i<R;i++){
            permutation;
        }
        processed_bits += rate_bits;
        processed_block++;
    }

    // 2. 处理最后的消息碎片并添加填充
    uint8_t last_block[265] = {0};
    int remaining_bits = msg_len_bits - processed_bits;
    int remaining_bytes = (remaining_bits + 7) / 8;
    
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
        if(digest_len_bits==512){
            absorb512(last_block,0);
        }else if(digest_len_bits==1024){
            absorb1024(last_block,0);
        }else if(digest_len_bits==768){
            absorb768(last_block,0);
        }
        for(int i=0;i<R;i++){
            permutation;
        }
        // 准备一个新的空块用于放置结束填充位 '1'
        memset(last_block, 0, rate_bytes);
    }

    // 在当前（或新的）块的最后一位添加结束填充 '1'
    last_block[rate_bytes - 1] |= 0x01;

    // 吸收最终包含填充的块
    if(digest_len_bits==512){
        absorb512(last_block,0);
    }else if(digest_len_bits==1024){
        absorb1024(last_block,0);
    }else if(digest_len_bits==768){
        absorb768(last_block,0);
    }
    for(int i=0;i<R;i++){
        permutation;
    }

    // 3. 输出阶段
    if(digest_len_bits==512){
        squeeze512(digest);
    }else if(digest_len_bits==1024){
        squeeze1024(digest);
    }else if(digest_len_bits==768){
        squeeze768(digest);
    }
    return 0;
}