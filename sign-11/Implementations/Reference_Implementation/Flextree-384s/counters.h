#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include "api.h"
#include "params.h"
#include "address.h"
#include "utils.h"


void save_wots_counter(uint32_t counter, unsigned char *sig, uint32_t merkle_tree_height);

uint32_t get_wots_counter(const unsigned char *sig,uint32_t merkle_tree_height);
