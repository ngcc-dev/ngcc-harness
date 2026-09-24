#include "../rans_byte.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define SCALE_BITS 10
#define SCALE (1 << SCALE_BITS)

#ifndef DARTS_MODE
#define DARTS_MODE 128
#endif

#if DARTS_MODE == 128

// 映射: -6到6 -> 0到12, -1023到-1019 -> 13到17, 1019到1023 -> 18到22
#define M_H 23 // Alphabet size for h
static const uint32_t f_h[M_H] = {
    // -6到6 (符号0-12)
    1, 1, 2, 20, 93, 234, 312, 234, 93, 20, 2, 1, 1, 
    // -1023到-1019 (符号13-17)
    1, 1, 1, 1, 1, 
    // 1019到1023 (符号18-22)
    1, 1, 1, 1, 1
};

// 映射: 0到5 -> 0到5, 1019到1023 -> 6到10
#define M_HB_Z1 11 // Alphabet size for highbits of z1
static const uint32_t f_hb_z1[11] = {334, 237, 89, 17, 1, 1, 1, 1, 17, 89, 237};

#elif DARTS_MODE == 256

// 映射: -7到7 -> 0到14, -511到-506 -> 15到20, 506到511 -> 21到26
#define M_H 27 // Alphabet size for h
static const uint32_t f_h[M_H] = {
    // -7到7 (符号0-14)
    1, 1, 1, 8, 37, 111, 215, 261, 216, 112, 38, 8, 1, 1, 1,
    // -511到-506 (符号15-20)
    1, 1, 1, 1, 1, 1,
    // 506到511 (符号21-26)
    1, 1, 1, 1, 1, 1
};

// 映射: 0到6 -> 0到6, 506到511 -> 7到12
#define M_HB_Z1 13 // Alphabet size for highbits of z1
static const uint32_t f_hb_z1[M_HB_Z1] = {278, 218, 110, 36, 7, 1, 1, 1, 1, 7, 36, 110, 218};

#elif DARTS_MODE == 512

// 映射: -18到18 -> 0到36, -1023到-1007 -> 37到53, 1007到1023 -> 54到70
#define M_H 71 // Alphabet size for h
static const uint32_t f_h[M_H] = {
    1, 1, 1, 1, 1, 1, 1, 2, 5, 8, 14, 22, 33, 46, 61, 75, 88, 97, 74, 97, 88, 75, 61, 46, 33, 22, 14, 8, 5, 2, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

// 映射: 0到17 -> 0到17, 1007到1023 -> 18到34
#define M_HB_Z1 35 // Alphabet size for highbits of z1
static const uint32_t f_hb_z1[M_HB_Z1] = {
    110, 97, 88, 75, 61, 46, 33, 22, 14, 8, 5, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 5, 8, 14, 22, 33, 46, 61, 75, 88, 97
};


#endif

static RansEncSymbol esyms_h[M_H];
static uint16_t symbol_h[SCALE] = {0};
static RansDecSymbol dsyms_h[M_H];

static RansEncSymbol esyms_hb_z1[M_HB_Z1];
static uint16_t symbol_hb_z1[SCALE] = {0};
static RansDecSymbol dsyms_hb_z1[M_HB_Z1];

/*************************************************
 * Name:        print_array
 *
 * Description: Print an array of uint32_t
 *
 * Arguments:   - uint32_t *array: pointer to the array
 *              - size_t length: length of the array
 **************************************************/
void print_array(uint32_t *array, size_t length) {
    for (size_t i = 0; i < length; i++) {
        printf("%u, ", array[i]);
    }
}



/**************************************************
void compute_freqs(uint32_t *freqs, const uint8_t *samples, int sample_size, int m) {
    double tmp;
    int i_max = 0;
    uint32_t freq_max = 0;
    int sum = 0;
    int diff;
    
    // 初始化
    for (int i = 0; i < m; i++) {
        freqs[i] = 0;
    }

    // 计算 Symbol
    for (int i = 0; i < sample_size; i++) {
        if (samples[i] >= m) 
            printf("Error: symbol out of range!");
        freqs[samples[i]]++;
    }

    // 归一化
    for (int i = 0; i < m; i++) {
        tmp = freqs[i] / (double) sample_size;
        tmp = tmp * (double) SCALE;
        freqs[i] = (uint32_t) tmp;
        
        if (freqs[i] ==0) // 频率至少为 1
            freqs[i] = 1;
        
        if (freqs[i] > freq_max) // 记录最大频率及其索引
        {
            freq_max = freqs[i];
            i_max = i;
        }
        
        sum += freqs[i]; // 计算总和
    }

    // 调整最大频率以确保总和为 SCALE
    diff = SCALE - sum;
    freqs[i_max] += diff;

    printf("Freq table with %u symbols, scales %u, diff is %i: {", m, SCALE, diff);
    print_array(freqs, m);
    printf("}\n");
}
 **************************************************/



/**************************************************
 * Name:        symbol_table
 *
 * Description: Create symbol table from frequency table
 *
 * Arguments:   - uint16_t *symbol: pointer to output symbol table
 *              - const uint32_t *freq: pointer to input frequency table
 *              - size_t alphabet_size: size of the alphabet
 **************************************************/
void symbol_table(uint16_t *symbol, const uint32_t *freq, size_t alphabet_size) {
    int pos = 0;
    for (size_t sym = 0; sym < alphabet_size; sym++) {
        for (uint32_t i = 0; i < freq[sym]; i++)
            symbol[pos++] = sym;
    }
}

/**************************************************
 * Name:        cum_freq_table
 *
 * Description: Create cumulative frequency table from frequency table
 *
 * Arguments:   - uint32_t *cum_freq: pointer to output cumulative frequency table
 *              - const uint32_t *freq: pointer to input frequency table
 *              - size_t alphabet_size: size of the alphabet
 **************************************************/
void cum_freq_table(uint32_t *cum_freq, const uint32_t *freq, size_t alphabet_size) {
    cum_freq[0] = 0;
    for (size_t i = 1; i < alphabet_size; i++) {
        cum_freq[i] = cum_freq[i - 1] + freq[i - 1];
    }
}

/**************************************************
 * Name:        encode_symbols
 *
 * Description: Create RansEncSymbol array from frequency table
 *
 * Arguments:   - RansEncSymbol *esyms: pointer to output RansEncSymbol array
 *              - const uint32_t *freq: pointer to input frequency table
 *              - size_t alphabet_size: size of the alphabet
 **************************************************/
void encode_symbols(RansEncSymbol *esyms, const uint32_t *freq, size_t alphabat_size) {
    uint32_t cum_freq[alphabat_size];
    cum_freq_table(cum_freq, freq, alphabat_size);

    for (size_t i = 0; i < alphabat_size; i++)
        RansEncSymbolInit(&esyms[i], cum_freq[i], freq[i], SCALE_BITS);
}

/**************************************************
 * Name:        decode_symbols
 *
 * Description: Create RansDecSymbol array from frequency table
 *
 * Arguments:   - RansDecSymbol *dsyms: pointer to output RansDecSymbol array
 *              - const uint32_t *freq: pointer to input frequency table
 *              - size_t alphabet_size: size of the alphabet
 **************************************************/
void decode_symbols(RansDecSymbol *dsyms, uint16_t *symbol, const uint32_t *freq, size_t alphabet_size) {
    uint32_t cum_freq[alphabet_size];
    cum_freq_table(cum_freq, freq, alphabet_size);

    symbol_table(symbol, freq, alphabet_size);
    
    for (size_t i=0; i < alphabet_size; i++) 
        RansDecSymbolInit(&dsyms[i], cum_freq[i], freq[i]);
}

/*************************************************
 * Name:        precomputations_rans
 *
 * Description: Precompute rANS encoding and decoding tables
 **************************************************/
void precomputations_rans() {
    encode_symbols(esyms_h, f_h, M_H);
    decode_symbols(dsyms_h, symbol_h, f_h, M_H);

    encode_symbols(esyms_hb_z1, f_hb_z1, M_HB_Z1);
    decode_symbols(dsyms_hb_z1, symbol_hb_z1, f_hb_z1, M_HB_Z1);
}


void print_esym(RansEncSymbol x){
    printf("{%i, %i, %i, %i, %i}", x.x_max, x.rcp_freq, x.bias, x.cmpl_freq, x.rcp_shift);
}

void print_dsym(RansDecSymbol x){
    printf("{%i, %i}", x.start, x.freq);
}

int main() {
    precomputations_rans();

    printf("\n");

    // encoding h
    printf("static RansEncSymbol esyms_h[M_H] = {");
    for(int i=0; i<M_H; i++) {
        print_esym(esyms_h[i]);
        printf(", ");
    }
    printf("};\n");

    printf("static RansDecSymbol dsyms_h[M_H] = {");
    for(int i=0; i<M_H; i++) {
        print_dsym(dsyms_h[i]);
        printf(", ");
    }
    printf("};\n");

    printf("static uint16_t symbol_h[SCALE] = {");
    for(int i=0; i< (int)SCALE; i++) {
        printf("%i, ", symbol_h[i]);
    }
    printf("};\n");

    // encoding HB(z1)
    printf("static RansEncSymbol esyms_hb_z1[M_HB_Z1] = {");
    for(int i=0; i<M_HB_Z1; i++) {
        print_esym(esyms_hb_z1[i]);
        printf(", ");
    }
    printf("};\n");

    printf("static RansDecSymbol dsyms_hb_z1[M_HB_Z1] = {");
    for(int i=0; i<M_HB_Z1; i++) {
        print_dsym(dsyms_hb_z1[i]);
        printf(", ");
    }
    printf("};\n");

    printf("static uint16_t symbol_hb_z1[SCALE] = {");
    for(int i=0; i< (int)SCALE; i++) {
        printf("%i, ", symbol_hb_z1[i]);
    }
    printf("};\n");

    printf("\n");
}