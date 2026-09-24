#include <stdint.h>

#include "params.h"
#include "counter.h"
#include "utils.h"

/* The GWOTS counter is stored just after the GWOTS signature and the tree authentication path*/
#define GWOTS_COUNTER_OFFSET (SPX_GWOTS_BYTES + SPX_TREE_HEIGHT * SPX_N)
/* TFORS  counter is stored just after the randomization value */
/* Maybe we dont need counter for TFORS here */
#define TFORS_COUNTER_OFFSET (SPX_N)

void save_gwots_counter(uint32_t counter, unsigned char *sig)
{
    ull_to_bytes(sig + GWOTS_COUNTER_OFFSET, COUNTER_SIZE, counter);
}

/* The counter is stored just after the GWOTS signature*/
uint32_t get_gwots_counter(const unsigned char *sig)
{
    return (uint32_t)bytes_to_ull(sig + GWOTS_COUNTER_OFFSET, COUNTER_SIZE);
}
/* TFORS  counter is stored in the end */
void save_tfors_counter(uint32_t counter, unsigned char *sig)
{
    ull_to_bytes(sig + TFORS_COUNTER_OFFSET, COUNTER_SIZE, counter);
}

uint32_t get_tfors_counter(const unsigned char *sig)
{
    return (uint32_t)bytes_to_ull(sig + TFORS_COUNTER_OFFSET, COUNTER_SIZE);
}