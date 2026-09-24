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

// 1. Iota step: Êµï¿½Ö·ï¿½Ê½Ò» ï¿½ï¿½ï¿½ï¿½Ö³ï¿½ï¿½ï¿½ï¿½ï¿½Ö±ï¿½Óµï¿½ï¿½Ã´ï¿½Ãµï¿½ï¿½ï¿½ï¿½ï¿½Ä·ï¿½Ê½ 
void iota_step(u64 state[25], int round) {
    state[0] ^= RC[round];
}

// 2. Pho step: laneÑ­ï¿½ï¿½ï¿½ï¿½Î»
void pho_step(u64 state[25]) {
    for(int i = 0; i < 25; i++) {
        state[i] = ROTR64(state[i], pho_rotc[i]);
    }
}


// 3. Theta step: Ë«ï¿½ï¿½ï¿½ï¿½Å¼Ð£ï¿½ï¿½ï¿½ï¿½É¢
void theta_step(u64 state[25]) {
    u64 P[5], Q[5], E[5], F[5];
    
    // ï¿½ï¿½ï¿½ï¿½ P ï¿½ï¿½ Q (ï¿½Ðµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½×ªï¿½ï¿½ï¿½)
    for(int x = 0; x < 5; x++) {
        P[x] = state[x] ^ state[x+5] ^ state[x+10] ^ state[x+15] ^ state[x+20];
        
        Q[x] = ROTR64(state[x], theta_rotc[0]) ^ 
               ROTR64(state[x+5], theta_rotc[1]) ^ 
               ROTR64(state[x+10], theta_rotc[2]) ^ 
               ROTR64(state[x+15], theta_rotc[3]) ^ 
               ROTR64(state[x+20], theta_rotc[4]);
    }
    
    // ï¿½ï¿½ï¿½ï¿½ï¿½Ð¼ï¿½ï¿½ï¿½ï¿½ E ï¿½ï¿½ F
    for(int x = 0; x < 5; x++) {
        E[x] = ROTR64(P[(x) % 5], u) ^ ROTR64(P[(x + 4) % 5], (u + r) % 64);
        F[x] = Q[(x + 1) % 5] ^ ROTR64(Q[(x + 3) % 5], s);
    }
    
    // ï¿½ï¿½ï¿½ï¿½×´Ì¬
    for(int x = 0; x < 5; x++) {
        for(int y = 0; y < 5; y++) {
            state[x + 5 * y] ^= E[x] ^ ROTR64(F[x], (theta_rotc[y] + u) % 64); 
        }
    }
}


// 4. Pi step: xyÆ½ï¿½ï¿½ï¿½Ã»ï¿½
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


// --- ï¿½ï¿½ï¿½Öºï¿½ï¿½ï¿½ ---
void Thunder_round(u64 state[25], int round_index) {
	//ï¿½ï¿½ï¿½ï¿½ï¿½Ó¡ï¿½zï¿½ï¿½Ñ­ï¿½ï¿½ï¿½ï¿½Î»ï¿½ï¿½Ë«ï¿½ï¿½ï¿½ï¿½Å¼Ð£ï¿½é¡¢xyï¿½ï¿½ï¿½Ã»ï¿½ï¿½ï¿½yï¿½ï¿½5ï¿½ï¿½ï¿½Ø·ï¿½ï¿½ï¿½ï¿½Ô²ï¿½ï¿½ï¿½ 
    iota_step(state, round_index);
    pho_step(state);
    theta_step(state);
    pi_step(state);
    chi_step(state);
}


//* Update */  Thunder_12r
void permutation(u64 state[25]) {

    // 12ï¿½ï¿½Ñ­ï¿½ï¿½
    for(int round = 0; round < 12; round++) {
		Thunder_round(state, round);
    }
    
}






//ï¿½ï¿½ï¿½Øµï¿½Ê±ï¿½ï¿½ï¿½Ç´ï¿½Ë¼ï¿½ï¿½Ø£ï¿½b[0]ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ð§ï¿½Ö½ï¿½ 
static u64 load64_be(const unsigned char *b) {
    return ((u64)b[0] << 56) | ((u64)b[1] << 48) | ((u64)b[2] << 40) | ((u64)b[3] << 32) |
           ((u64)b[4] << 24) | ((u64)b[5] << 16) | ((u64)b[6] << 8)  | ((u64)b[7]);
}

//ï¿½ï¿½ï¿½ï¿½ï¿½Ê±ï¿½ï¿½ï¿½Ç´ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½b[0]ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ð§ï¿½Ö½ï¿½ 
static void store64_be(unsigned char *b, u64 w) {
    b[0] = (unsigned char)(w >> 56); b[1] = (unsigned char)(w >> 48);
    b[2] = (unsigned char)(w >> 40); b[3] = (unsigned char)(w >> 32);
    b[4] = (unsigned char)(w >> 24); b[5] = (unsigned char)(w >> 16);
    b[6] = (unsigned char)(w >> 8);  b[7] = (unsigned char)w;
}


static void copy_bits_msb(unsigned char *dst, unsigned long long dst_bit_offset,
                          const unsigned char *src, unsigned int bit_len) {
    for (unsigned int bit = 0; bit < bit_len; bit++) {
        unsigned char src_bit = (unsigned char)((src[bit / 8] >> (7 - (bit % 8))) & 1U);
        unsigned long long dst_bit = dst_bit_offset + bit;
        unsigned char mask = (unsigned char)(1U << (7 - (dst_bit % 8)));

        if (src_bit) {
            dst[dst_bit / 8] |= mask;
        } else {
            dst[dst_bit / 8] &= (unsigned char)~mask;
        }
    }
}

static void extract_squeeze_block(u64 X[25], unsigned char *block) {
    for (int i = 0; i < SQUEEZE_LANE_NUM; i++) {
        int idx = SQUEEZE_LANE_NUM - 1 - i;
        store64_be(block + (i * 8), X[idx]);
    }
}

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{   

	//Ô­Ê¼ï¿½ï¿½Ï¢ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½bit 
	//rem_bits ï¿½ï¿½Ê£ï¿½ï¿½ï¿½ï¿½ bit Ã»ï¿½Ð±ï¿½ï¿½ï¿½ï¿½Õ´ï¿½ï¿½ï¿½ 
    unsigned long long rem_bits = msg_len_bits;
    int i;
    u64 S[IV_LANE_NUM];
    
    /* --- 1. Initialize Phase --- */
    u64 X[25] = {0};
    
    // X[0]ï¿½ï¿½ï¿½ï¿½ï¿½Î»ï¿½ï¿½IVï¿½ï¿½ï¿½ï¿½ï¿½Î»ï¿½Åµï¿½X[0] 
    for (i = 0; i < IV_LANE_NUM; i++) {
        X[i] = IV[IV_LANE_NUM - 1 - i];
    }
	
	//RATE_BITS  Ò»ï¿½ï¿½ï¿½ï¿½ï¿½Õ¶ï¿½ï¿½ï¿½ï¿½ï¿½Ï¢   rateï¿½ï¿½ï¿½ÖµÄ³ï¿½ï¿½ï¿½ 
    /* --- 2. Absorbing Phase --- */
    while (rem_bits >= RATE_BITS) {
    	
    	//ï¿½ï¿½ï¿½ï¿½ï¿½Â°ë²¿ï¿½Öµï¿½×´Ì¬Öµï¿½ï¿½ÎªÇ°ï¿½ï¿½ï¿½ï¿½×¼ï¿½ï¿½ï¿½ï¿½X[0] -> S[-1] 
        for (i = 0; i < IV_LANE_NUM; i++) {
            S[IV_LANE_NUM - 1 - i] = X[i];
        }
        
        //ï¿½Ï°ë²¿ï¿½Ö£ï¿½ï¿½ï¿½X[24]ï¿½ï¿½Ê¼ï¿½ï¿½ï¿½ï¿½Ï¢Öµ 
        for (i = 0; i < MSG_LANE_NUM; i++) {
           int idx = 24 - i;
            X[idx] ^= load64_be(msg + (i * 8));
        }

		//ï¿½Ã»ï¿½ 
        permutation(X);
		
		//Ç°ï¿½ï¿½ X[0]^ S[-1]  (Ö®Ç°ï¿½ï¿½S[0])
        for (i = 0; i < IV_LANE_NUM; i++) {
            X[i] ^= S[IV_LANE_NUM - 1 - i];
        }
        msg += RATE_BYTES;
        rem_bits -= RATE_BITS;
    }


    int need_extra_zero_block = (rem_bits == RATE_BITS - 1ULL);

    /* --- 3. Padding pd10* --- */  
    //RATE_BYTES ï¿½ï¿½ï¿½ï¿½ï¿½Ö½Úµï¿½ï¿½ï¿½ï¿½Õ³ï¿½ï¿½ï¿½ 
    // Ê£ï¿½ÂµÄ²ï¿½ï¿½ï¿½rateï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½0 
    //ï¿½ï¿½ï¿½ï¿½Ò»ï¿½ï¿½mï¿½Ö½Úµï¿½ï¿½ï¿½ï¿½ï¿½ 
    unsigned char pad_block[RATE_BYTES] = {0};
    
    //ï¿½ï¿½ï¿½ï¿½Ê£ï¿½ï¿½ï¿½Ö½ï¿½/ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ 
    unsigned int full_bytes = (unsigned int)(rem_bits / 8);
    unsigned int partial_bits = (unsigned int)(rem_bits % 8);
    
	//pad_block ï¿½ï¿½[0]ï¿½ï¿½ï¿½ï¿½Î»ï¿½Ã¿ï¿½Ê¼ï¿½Å·ï¿½0ï¿½Ö½ï¿½ 
    if (full_bytes > 0) {
        memcpy(pad_block, msg, full_bytes);
    }
	
	//ï¿½ï¿½ï¿½Õ¸ï¿½Î»ï¿½Å±ï¿½ï¿½ï¿½eg  111 ï¿½ï¿½ï¿½ï¿½ 11100000 
    if (partial_bits > 0) {
        pad_block[full_bytes] = msg[full_bytes] & (0xFFU << (8 - partial_bits));
        pad_block[full_bytes] |= (0x80U >> partial_bits);
    } else {
    	
    	//ï¿½ï¿½ï¿½Ã±ß½ï¿½Ä»ï¿½ï¿½ï¿½Ò»ï¿½ï¿½1 
        pad_block[full_bytes] = 0x80U;
    }

    /* --- 4. last block --- */
    for (i = 0; i < MSG_LANE_NUM; i++) {
        int idx = 24 - i;
        
        //ï¿½ï¿½X[24]ï¿½ï¿½Ê¼ï¿½ï¿½ï¿½ï¿½ï¿½Ï¢ 
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
     * ½öµ± rem_bits = RATE_BITS - 1 Ê±Ö´ÐÐ¡£
     * Õâ¸ö¶îÍâ block ÊÇ r bit È« 0¡£
     * ÒòÎª XOR È« 0 ²»¸Ä±ä rate Çø£¬ËùÒÔÕâÀï²»ÐèÒªÔÙ¶Ô X[24], X[23], ... ×öÒì»ò¡£
     * µ«ËüÈÔÈ»×÷ÎªÒ»¸öÎüÊÕ¿é£¬ÐèÒªÖ´ÐÐ permutation ºÍ feed-forward¡£
     */
    if (need_extra_zero_block) {


	    X[0] ^= 0x01ULL;

        // ±£´æÇ°À¡×´Ì¬
        for (i = 0; i < IV_LANE_NUM; i++) {
            S[IV_LANE_NUM - 1 - i] = X[i];
        }
        // ÎüÊÕÈ« 0 block£ºX[idx] ^= 0£¬ËùÒÔÕâÀïÊ²Ã´¶¼²»ÓÃ×ö
        permutation(X);
        // Ç°À¡
        for (i = 0; i < IV_LANE_NUM; i++) {
            X[i] ^= S[IV_LANE_NUM - 1 - i];
        }
    }





    /* --- 5. Squeezing Phase --- */

    unsigned long long produced_bits = 0;
    unsigned long long requested_bits = (unsigned long long)digest_len_bits;
    unsigned long long digest_bytes = (requested_bits + 7) / 8;

    unsigned char squeeze_block[SQUEEZE_BIT_LENGTH / 8];

    if (digest_bytes > 0) {
        memset(digest, 0, (size_t)digest_bytes);
    }

    while (produced_bits < requested_bits) {
        unsigned long long remaining_bits = requested_bits - produced_bits;

        unsigned int take_bits =
            remaining_bits < SQUEEZE_BIT_LENGTH
                ? (unsigned int)remaining_bits
                : SQUEEZE_BIT_LENGTH;

        /*
            ä¸€ç»´ squeezeï¼š
                X[idx]
        */
        extract_squeeze_block(X, squeeze_block);

        copy_bits_msb(digest, produced_bits, squeeze_block, take_bits);

        produced_bits += take_bits;

        /*
            å¦‚æžœè¿˜éœ€è¦ç»§ç»­è¾“å‡ºï¼Œåˆ™ç»§ç»­ç½®æ¢å¹¶ feed-forwardã€‚
        */
        if (produced_bits < requested_bits) {
            for (i = 0; i < IV_LANE_NUM; i++) {
                S[IV_LANE_NUM - 1 - i] = X[i];
            }

            permutation(X);

            for (i = 0; i < IV_LANE_NUM; i++) {
                X[i] ^= S[IV_LANE_NUM - 1 - i];
            }
        }
    }
    


    return 0;
}
