#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include "api.h"
#include "params.h"
#include "address.h"
#include "utils.h"


void save_gwots_counter(uint32_t counter, unsigned char *sig);

uint32_t get_gwots_counter(const unsigned char *sig);

void save_fors_counter(uint32_t counter_bytes, unsigned char *sig);

uint32_t get_fors_counter(const unsigned char *sig);