#include "universal_hash.h"

extern inline void hasher_gfsecpar_init_key(hasher_gfsecpar_key* hash_key, poly_secpar_vec key);
extern inline void hasher_gfsecpar_init_state(hasher_gfsecpar_state* state, size_t num_coefficients);
extern inline void hasher_gfsecpar_update(const hasher_gfsecpar_key* key, hasher_gfsecpar_state* state, poly_secpar_vec input);
extern inline poly_secpar_vec hasher_gfsecpar_final(const hasher_gfsecpar_state* state);
extern inline void hasher_gfsecpar_64_init_key(hasher_gfsecpar_64_key* hash_key, poly64_vec key);
extern inline void hasher_gfsecpar_64_init_state(hasher_gfsecpar_64_state* state, size_t num_coefficients);
extern inline void hasher_gfsecpar_64_update(const hasher_gfsecpar_64_key* key, hasher_gfsecpar_64_state* state, poly_secpar_vec input);
extern inline poly_secpar_vec hasher_gfsecpar_64_final(const hasher_gfsecpar_64_state* state);
extern inline poly_secpar_vec gfsecpar_combine_hashes(poly_secpar_vec key_exp, poly_secpar_vec hash);
extern inline void hasher_gf64_init_key(hasher_gf64_key* hash_key, poly64_vec key);
extern inline void hasher_gf64_init_state(hasher_gf64_state* state, size_t num_coefficients);
extern inline void hasher_gf64_update(const hasher_gf64_key* key, hasher_gf64_state* state, poly64_vec input);
extern inline poly64_vec hasher_gf64_final(const hasher_gf64_state* state);
extern inline poly64_vec gf64_combine_hashes(poly64_vec key_exp, poly64_vec hash);
