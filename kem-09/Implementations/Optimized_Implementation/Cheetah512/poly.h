#ifndef CHEETAH_POLY_H
#define CHEETAH_POLY_H


void sample_vector(const uint8_t *buf, int16_t *vec, size_t len);
void sample_noise_vector(const uint8_t *buf, int16_t *noisevec, size_t len);

void poly_vec_add(const int16_t *a, const int16_t *b, int16_t *c, size_t k);
void poly_mat_vec_pwmul(const int16_t *A, const int16_t *x, int16_t *y);
void poly_vec_mat_pwmul(const int16_t *x, const int16_t *A, int16_t *y);
void poly_vec_pwmul(const int16_t *a, const int16_t *b, int16_t *c);

void vector_mul_2(int16_t *vec, size_t len);
void vector_centered_mod(int16_t *vec, size_t len) ;
void vector_mod_2(int16_t *vec, size_t len) ; 

void compress(const int16_t *vec, size_t len, int16_t *out, uint8_t dbits);
void decompress(const int16_t *vec, size_t len, int16_t *out, uint8_t dbits);

void encode_vector(const int16_t *in,  uint8_t *out, size_t inlen, uint8_t dbits);
void decode_vector(const uint8_t *in, int16_t *out, size_t outlen, uint8_t dbits);
void encode_8bit_lsb(const int16_t *data, size_t datalen, uint8_t *out);
void decode_8bit_lsb(const uint8_t *in, size_t outlen, int16_t *out); 
void encode_11bit_lsb(const int16_t* input, size_t n, uint8_t* output);
void decode_11bit_lsb(const uint8_t* input, size_t n, int16_t* output);
void encode_13bit_lsb(const uint16_t *data, size_t datalen, uint8_t *out);
void decode_13bit_lsb(const uint8_t *in, size_t outlen, uint16_t *out);

void encode_msg(const int16_t *msgpoly, uint8_t msg[MSG_BYTES]) ;
void decode_msg(const uint8_t msg[MSG_BYTES], int16_t *msgpoly) ;

#endif