#include <stdint.h>
#include <string.h>

#include "address.h"
#include "params.h"
#include "counters.h"
#include "api.h"


/* FORS+C  counter is stored just after the randomization value */
#define FORS_COUNTER_OFFSET (SPX_N)


void save_wots_counter(uint32_t counter, unsigned char *sig,uint32_t merkle_tree_height) {
    unsigned char counter_bytes[COUNTER_SIZE];

    ull_to_bytes(counter_bytes, COUNTER_SIZE, counter);
    memcpy(sig + (SPX_WOTS_BYTES + merkle_tree_height * SPX_N), counter_bytes, COUNTER_SIZE);
}

/* The counter is stored just after the WOTS signature*/
uint32_t get_wots_counter(const unsigned char *sig,uint32_t merkle_tree_height) {
    uint32_t counter;
    unsigned char counter_bytes[COUNTER_SIZE];

    memcpy(counter_bytes, sig + (SPX_WOTS_BYTES + merkle_tree_height * SPX_N), COUNTER_SIZE);
    counter = bytes_to_ull(counter_bytes, COUNTER_SIZE);
    return counter;
}


/* FORS+C  counter is stored in the end */
void save_fors_counter(uint32_t counter, unsigned char *sig) {
    unsigned char counter_bytes[COUNTER_SIZE];

    ull_to_bytes(counter_bytes,COUNTER_SIZE,counter);
    memcpy(sig + FORS_COUNTER_OFFSET, counter_bytes, COUNTER_SIZE);
}


uint32_t get_fors_counter(const unsigned char *sig)
{
    unsigned char counter_bytes_out[COUNTER_SIZE];
    uint32_t counter;

    memcpy(counter_bytes_out, sig + FORS_COUNTER_OFFSET, COUNTER_SIZE);
    counter = bytes_to_ull(counter_bytes_out, COUNTER_SIZE);
    return counter;
}
