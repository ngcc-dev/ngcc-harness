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
#include <string.h>


/* --- 1. Permutation Declaration and Definition -*/

#define TOTAL_ROUNDS 12
/* Round Constants */
static const u64 RC[12] = {
    0x0001ULL, 0x0003ULL, 0x000AULL, 0x0088ULL, 
    0x8081ULL, 0x8002ULL, 0x0009ULL, 0x0083ULL, 
    0x800AULL, 0x0089ULL, 0x8083ULL, 0x800BULL
};


/* PHO Constants */
static const int pho_rotc[25] = {
    // y = 0: x=0,1,2,3,4
    45, 33, 13,  1, 27, 
    // y = 1: x=0,1,2,3,4
    40, 52, 19, 47,  9, 
    // y = 2: x=0,1,2,3,4
    34,  5,  0,  2, 37, 
    // y = 3: x=0,1,2,3,4
    48, 30, 61,  8, 38, 
    // y = 4: x=0,1,2,3,4
     6, 18, 49, 17, 43  
};
	
/* PI Constants */
static const int pi_piln[25] = {
	// y = 0: x=0,1,2,3,4
    4,  5, 11, 17, 23, 
    // y = 1: x=0,1,2,3,4
    2,  8, 14, 15, 21, 
    // y = 2: x=0,1,2,3,4
    0,  6, 12, 18, 24, 
    // y = 3: x=0,1,2,3,4
    3,  9, 10, 16, 22, 
    // y = 4: x=0,1,2,3,4
    1,  7, 13, 19, 20
};
	
/* THETA Constants */
static const int theta_rotc[5] = {41, 50, 6, 10, 27};
static const int u = 26, r = 1, s = 36; 


/* Right Rotation Function */
static u64 ROTR64(u64 x, int shift) {
    if (shift == 0) return x;
    return ((x >> shift) | (x << (64 - shift)));
}

// 1. Iota step: 实现方式一 异或轮常数，直接调用存好的数组的方式 
void iota_step(u64 state[25], int round) {
    state[0] ^= RC[round];
}

// 2. Pho step: lane循环移位
void pho_step(u64 state[25]) {
    for(int i = 0; i < 25; i++) {
        state[i] = ROTR64(state[i], pho_rotc[i]);
    }
}


// 3. Theta step: 双列奇偶校验扩散
void theta_step(u64 state[25]) {
    u64 P[5], Q[5], E[5], F[5];
    
    // 计算 P 和 Q (列的异或与旋转异或)
    for(int x = 0; x < 5; x++) {
        P[x] = state[x] ^ state[x+5] ^ state[x+10] ^ state[x+15] ^ state[x+20];
        
        Q[x] = ROTR64(state[x], theta_rotc[0]) ^ 
               ROTR64(state[x+5], theta_rotc[1]) ^ 
               ROTR64(state[x+10], theta_rotc[2]) ^ 
               ROTR64(state[x+15], theta_rotc[3]) ^ 
               ROTR64(state[x+20], theta_rotc[4]);
    }
    
    // 计算中间变量 E 和 F
    for(int x = 0; x < 5; x++) {
        E[x] = ROTR64(P[(x) % 5], u) ^ ROTR64(P[(x + 4) % 5], (u + r) % 64);
        F[x] = Q[(x + 1) % 5] ^ ROTR64(Q[(x + 3) % 5], s);
    }
    
    // 更新状态
    for(int x = 0; x < 5; x++) {
        for(int y = 0; y < 5; y++) {
            state[x + 5 * y] ^= E[x] ^ ROTR64(F[x], (theta_rotc[y] + u) % 64); 
        }
    }
}


// 4. Pi step: xy平面置换
//{D}[y][(2x+3y+2)][z] = {D}[x][y][z]
void pi_step(u64 state[25]) {
    u64 tmp[25];
    for(int i = 0; i < 25; i++) {
        tmp[i] = state[pi_piln[i]];
    }
    for(int i = 0; i < 25; i++) {
        state[i] = tmp[i];
    }
}


void chi_step(u64 state[25]) {
    for(int j = 0; j < 5; j++) {
        u64 t0 = state[j];
        u64 t1 = state[j+5];
        u64 t2 = state[j+10];
        u64 t3 = state[j+15];
        u64 t4 = state[j+20];
        
        state[j]    = t3 ^ ((~t1) & t2 & (~t4));
        state[j+5]  = t4 ^ ((~t2) & t3 & (~t0));
        state[j+10] = t0 ^ ((~t3) & t4 & (~t1));
        state[j+15] = t1 ^ ((~t4) & t0 & (~t2));
        state[j+20] = t2 ^ ((~t0) & t1 & (~t3));
    }
}


// --- 单轮函数 ---
void Thunder_round(u64 state[25], int round_index) {
	//常数加、z轴循环移位、双列奇偶校验、xy面置换、y上5比特非线性操作 
    iota_step(state, round_index);
    pho_step(state);
    theta_step(state);
    pi_step(state);
    chi_step(state);
}


//* Update */  Thunder_12r
void permutation(u64 state[25]) {

    // 12轮循环
    for(int round = 0; round < 12; round++) {
		Thunder_round(state, round);
    }
    
}


//加载的时候是大端加载，b[0]是最高有效字节 
static u64 load64_be(const unsigned char *b) {
    return ((u64)b[0] << 56) | ((u64)b[1] << 48) | ((u64)b[2] << 40) | ((u64)b[3] << 32) |
           ((u64)b[4] << 24) | ((u64)b[5] << 16) | ((u64)b[6] << 8)  | ((u64)b[7]);
}

//输出的时候是大端输出，b[0]是最高有效字节 
static void store64_be(unsigned char *b, u64 w) {
    b[0] = (unsigned char)(w >> 56); b[1] = (unsigned char)(w >> 48);
    b[2] = (unsigned char)(w >> 40); b[3] = (unsigned char)(w >> 32);
    b[4] = (unsigned char)(w >> 24); b[5] = (unsigned char)(w >> 16);
    b[6] = (unsigned char)(w >> 8);  b[7] = (unsigned char)w;
}

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{   

	//原始消息长度 按照bit 
	//rem_bits 还剩多少 bit 没有被吸收处理 
    unsigned long long rem_bits = msg_len_bits;
    int i;
    u64 S[IV_LANE_NUM];
    
    /* --- 1. Initialize Phase --- */
    u64 X[25] = {0};
    
    // X[0]是最低位，IV的最高位放到X[0] 
    for (i = 0; i < IV_LANE_NUM; i++) {
        X[i] = IV[IV_LANE_NUM - 1 - i];
    }
	
	//RATE_BITS  一次吸收多少消息   rate部分的长度 
    /* --- 2. Absorbing Phase --- */
    while (rem_bits >= RATE_BITS) {
    	
    	//储存下半部分的状态值，为前馈做准备，X[0] -> S[-1] 
        for (i = 0; i < IV_LANE_NUM; i++) {
            S[IV_LANE_NUM - 1 - i] = X[i];
        }
        
        //上半部分，从X[24]开始放消息值 
        for (i = 0; i < MSG_LANE_NUM; i++) {
           int idx = 24 - i;
            X[idx] ^= load64_be(msg + (i * 8));
        }

		//置换 
        permutation(X);
		
		//前馈 X[0]^ S[-1]  (之前的S[0])
        for (i = 0; i < IV_LANE_NUM; i++) {
            X[i] ^= S[IV_LANE_NUM - 1 - i];
        }
        msg += RATE_BYTES;
        rem_bits -= RATE_BITS;
    }


    /* 
     * 特殊情况标志：
     * 如果剩余消息只差 1 bit 就满一个 rate block，
     * 那么补一个 1 后刚好满 block。
     * 按你的新要求，这种情况下还要额外再吸收一个 r-bit 全 0 block。
     */
    int need_extra_zero_block = (rem_bits == RATE_BITS - 1ULL);
    
    

    /* --- 3. Padding pd10* --- */  
    //RATE_BYTES 按照字节的吸收长度 
    // 剩下的不足rate长度 补0 
    //生成一个m字节的数组 
    unsigned char pad_block[RATE_BYTES] = {0};
    
    //计算剩余字节/比特数 
    unsigned int full_bytes = (unsigned int)(rem_bits / 8);
    unsigned int partial_bits = (unsigned int)(rem_bits % 8);
    
	//pad_block 从[0]索引位置开始放非0字节 
    if (full_bytes > 0) {
        memcpy(pad_block, msg, full_bytes);
    }
	
	//按照高位放比特eg  111 则是 11100000 
    if (partial_bits > 0) {
        pad_block[full_bytes] = msg[full_bytes] & (0xFFU << (8 - partial_bits));
        pad_block[full_bytes] |= (0x80U >> partial_bits);
    } else {
    	
    	//正好边界的话补一个1 
        pad_block[full_bytes] = 0x80U;
    }

    /* --- 4. last block --- */
    for (i = 0; i < MSG_LANE_NUM; i++) {
        int idx = 24 - i;
        
        //从X[24]开始吸收消息 
        X[idx] ^= load64_be(pad_block + (i * 8));
    }
    
	
	X[0] ^= !need_extra_zero_block;
	 
    for (i = 0; i < IV_LANE_NUM; i++) {
        S[IV_LANE_NUM - 1 - i] = X[i];
    }

    permutation(X);

    for (i = 0; i < IV_LANE_NUM; i++) {
        X[i] ^= S[IV_LANE_NUM - 1 - i];
    }
    
    
    
        /*
     * --- 4.1 Extra all-zero block ---
     *
     * 仅当 rem_bits = RATE_BITS - 1 时执行。
     * 这个额外 block 是 r bit 全 0。
     * 因为 XOR 全 0 不改变 rate 区，所以这里不需要再对 X[24], X[23], ... 做异或。
     * 但它仍然作为一个吸收块，需要执行 permutation 和 feed-forward。
     */
    if (need_extra_zero_block) {


	    X[0] ^= 0x01ULL;

        // 保存前馈状态
        for (i = 0; i < IV_LANE_NUM; i++) {
            S[IV_LANE_NUM - 1 - i] = X[i];
        }
        // 吸收全 0 block：X[idx] ^= 0，所以这里什么都不用做
        permutation(X);
        // 前馈
        for (i = 0; i < IV_LANE_NUM; i++) {
            X[i] ^= S[IV_LANE_NUM - 1 - i];
        }
    }
    
    
    /* --- 5. Squeezing Phase --- */ 
    for (i = 0; i < DIGEST_LANE_NUM; i++) {
        int idx = DIGEST_LANE_NUM - 1 - i;
        store64_be(digest + (i * 8), X[idx]);
    }

    return 0;
}
