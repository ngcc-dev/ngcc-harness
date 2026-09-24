#include "rain.h"

#include <assert.h>

void rain_encrypt_block(uint64_t* block, const uint64_t* key) {
    #if defined(OWF_RAIN_3)
    #if SECURITY_PARAM == 128
    block128 sbox_out;
    rain_round_function(block, key, (uint64_t*)&rain_rc_128[0], (uint64_t*)&rain_mat_128[0*128], (uint64_t*)&sbox_out);
    rain_round_function(block, key, (uint64_t*)&rain_rc_128[1], (uint64_t*)&rain_mat_128[1*128], (uint64_t*)&sbox_out);
    rain_last_round_function(block, key, (uint64_t*)&rain_rc_128[2]);
    #elif SECURITY_PARAM == 192
    block192 sbox_out;
    rain_round_function(block, key, (uint64_t*)&rain_rc_192[0], (uint64_t*)&rain_mat_192[0*192], (uint64_t*)&sbox_out);
    rain_round_function(block, key, (uint64_t*)&rain_rc_192[1], (uint64_t*)&rain_mat_192[1*192], (uint64_t*)&sbox_out);
    rain_last_round_function(block, key, (uint64_t*)&rain_rc_192[2]);
    #elif SECURITY_PARAM == 256
    block256 sbox_out;
    rain_round_function(block, key, (uint64_t*)&rain_rc_256[0], (uint64_t*)&rain_mat_256[0*256], (uint64_t*)&sbox_out);
    rain_round_function(block, key, (uint64_t*)&rain_rc_256[1], (uint64_t*)&rain_mat_256[1*256], (uint64_t*)&sbox_out);
    rain_last_round_function(block, key, (uint64_t*)&rain_rc_256[2]);
    #endif
    #elif defined(OWF_RAIN_4)
    #if SECURITY_PARAM == 128
    block128 sbox_out;
    rain_round_function(block, key, (uint64_t*)&rain_rc_128[0], (uint64_t*)&rain_mat_128[0*128], (uint64_t*)&sbox_out);
    rain_round_function(block, key, (uint64_t*)&rain_rc_128[1], (uint64_t*)&rain_mat_128[1*128], (uint64_t*)&sbox_out);
    rain_round_function(block, key, (uint64_t*)&rain_rc_128[2], (uint64_t*)&rain_mat_128[2*128], (uint64_t*)&sbox_out);
    rain_last_round_function(block, key, (uint64_t*)&rain_rc_128[3]);
    #elif SECURITY_PARAM == 192
    block192 sbox_out;
    rain_round_function(block, key, (uint64_t*)&rain_rc_192[0], (uint64_t*)&rain_mat_192[0*192], (uint64_t*)&sbox_out);
    rain_round_function(block, key, (uint64_t*)&rain_rc_192[1], (uint64_t*)&rain_mat_192[1*192], (uint64_t*)&sbox_out);
    rain_round_function(block, key, (uint64_t*)&rain_rc_192[2], (uint64_t*)&rain_mat_192[2*192], (uint64_t*)&sbox_out);
    rain_last_round_function(block, key, (uint64_t*)&rain_rc_192[3]);
    #elif SECURITY_PARAM == 256
    block256 sbox_out;
    rain_round_function(block, key, (uint64_t*)&rain_rc_256[0], (uint64_t*)&rain_mat_256[0*256], (uint64_t*)&sbox_out);
    rain_round_function(block, key, (uint64_t*)&rain_rc_256[1], (uint64_t*)&rain_mat_256[1*256], (uint64_t*)&sbox_out);
    rain_round_function(block, key, (uint64_t*)&rain_rc_256[2], (uint64_t*)&rain_mat_256[2*256], (uint64_t*)&sbox_out);
    rain_last_round_function(block, key, (uint64_t*)&rain_rc_256[3]);
    #endif  
    #endif
}

