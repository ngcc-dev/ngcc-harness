#ifndef LOONG_POLY_H
#define LOONG_POLY_H


void sample_vector(const uint8_t *buf, int16_t *vec, size_t len);
void sample_noise_vector(const uint8_t *buf, int16_t *noisevec, size_t len);

void poly_vec_inner_product(const int16_t *x, const int16_t *y, int16_t *result, size_t len);
void poly_mat_vec_mul(const int16_t *A, const int16_t *x, int16_t *y, int nrows, int ncols);
void poly_vec_mat_mul(int16_t *x, int16_t *A, int16_t *y, size_t nrows, size_t ncols);
void poly_vec_add(const int16_t *a, const int16_t *b, int16_t *c, size_t k);

void vector_mul_2(int16_t *vec, size_t len);
void vector_central_modulo_q(int16_t *vec, size_t len) ;
void vector_mod_2(int16_t *vec, size_t len) ; 

void poly_to_negacyclic_matrix(const int16_t *poly, int16_t *matrix);
void polys_to_block_negacyclic_matrix(const int16_t *vec, int16_t *mat, int k1, int k2); 

void mat_add(const int16_t *A, const int16_t *B, int16_t *C, int nrows, int ncols);
void mat_sub(const int16_t *A, const int16_t *B, int16_t *C, int nrows, int ncols);
void mat_mul(const int16_t *A, const int16_t *B, int16_t *C, int nrows, int ncols, int nout);

void compress(const int16_t *vec, size_t len, int16_t *out, uint8_t dbits);
void decompress(const int16_t *vec, size_t len, int16_t *out, uint8_t dbits);

void encode_vector(const int16_t *in,  uint8_t *out, size_t inlen, uint8_t dbits);
void decode_vector(const uint8_t *in, int16_t *out, size_t outlen, uint8_t dbits);

void encode_4bit_lsb(const int16_t *data, size_t n, uint8_t *out);
void decode_4bit_lsb(const uint8_t *in, size_t n, int16_t *out);
void encode_6bit_lsb(const int16_t *data, size_t n, uint8_t *out);
void decode_6bit_lsb(const uint8_t *in, size_t n, int16_t *out);
void encode_10bit_lsb(const int16_t *data, size_t n, uint8_t *out);
void decode_10bit_lsb(const uint8_t *in, size_t n, int16_t *out);
void encode_11bit_lsb(const int16_t *data, size_t n, uint8_t *out);
void decode_11bit_lsb(const uint8_t *in, size_t n, int16_t *out);

void encode_noise_vector(const int16_t * in, uint8_t* out, size_t inlen);
void decode_noise_vector(const uint8_t* in, int16_t* out, size_t outlen);

void encode_msg(const int16_t *mat, uint8_t msg[MSG_BYTES]) ;
void decode_msg(const uint8_t msg[MSG_BYTES], int16_t *mat) ;

#endif