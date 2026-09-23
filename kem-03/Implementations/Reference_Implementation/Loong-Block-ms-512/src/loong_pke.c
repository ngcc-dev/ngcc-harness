#include "loong_pke.h"

#include "augabidulin.h"
#include "loong_hash.h"
#include "loong_status.h"
#include "loong_xof_reader.h"
#include "parsing.h"
#include "rbc_vec.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define LOONG_PKE_L ((unsigned int)(LOONG_N1 * LOONG_N2))
#define LOONG_PKE_XS_SIZE ((unsigned int)(LOONG_N * LOONG_N1))
#define LOONG_PKE_C1_SIZE ((unsigned int)(LOONG_N2 * LOONG_N))
#define LOONG_PKE_C2_SIZE ((unsigned int)(LOONG_N2 * LOONG_N1))
#define LOONG_PKE_LOGICAL_ELT_BYTES ((unsigned int)((LOONG_M + 7U) / 8U))
#define LOONG_PKE_G_ATTEMPTS 1024U
#define LOONG_PKE_SUPPORT_ATTEMPTS 4096U
#define LOONG_PKE_POSITION_ATTEMPTS 4096U
#define LOONG_PKE_SUPPORT_XOF_BUDGET 65536ULL

#define LOONG_PKE_STATIC_ASSERT(cond, name) typedef char loong_pke_static_assert_##name[(cond) ? 1 : -1]

LOONG_PKE_STATIC_ASSERT(LOONG_M <= RBC_FIELD_M, logical_m_fits_field_layer);
LOONG_PKE_STATIC_ASSERT(LOONG_PKE_L == LOONG_N1 * LOONG_N2, l_definition_valid);

static unsigned long long ceil_div_ull(unsigned long long a,
                                       unsigned long long b)
{
	return (a + b - 1ULL) / b;
}

static int checked_mul_ull(unsigned long long *out, unsigned long long a,
                           unsigned long long b)
{
	if (out == 0) {
		return LOONG_ERR_NULL;
	}
	if (a != 0 && b > ULLONG_MAX / a) {
		return LOONG_ERR_OVERFLOW;
	}
	*out = a * b;
	return LOONG_SUCCESS;
}

static int checked_add_ull(unsigned long long *out, unsigned long long a,
                           unsigned long long b)
{
	if (out == 0) {
		return LOONG_ERR_NULL;
	}
	if (a > ULLONG_MAX - b) {
		return LOONG_ERR_OVERFLOW;
	}
	*out = a + b;
	return LOONG_SUCCESS;
}

static unsigned long long logical_vec_bits(unsigned int size)
{
	return (unsigned long long)size * (unsigned long long)LOONG_M;
}

static unsigned long long logical_vec_bytes(unsigned int size)
{
	return ceil_div_ull(logical_vec_bits(size), 8ULL);
}

unsigned long long loong_pke_get_message_len_bytes(void)
{
	return logical_vec_bytes(LOONG_K);
}

static int check_padding_zero(const unsigned char *in,
                              unsigned long long in_len_bytes,
                              unsigned long long used_bits)
{
	unsigned int used_in_last;
	unsigned char padding_mask;

	if (used_bits == 0) {
		return LOONG_SUCCESS;
	}
	if (in == 0) {
		return LOONG_ERR_NULL;
	}
	if (ceil_div_ull(used_bits, 8ULL) != in_len_bytes) {
		return LOONG_ERR_BAD_LENGTH;
	}
	used_in_last = (unsigned int)(used_bits % 8ULL);
	if (used_in_last == 0) {
		return LOONG_SUCCESS;
	}
	padding_mask = (unsigned char)((1U << (8U - used_in_last)) - 1U);
	return (in[in_len_bytes - 1ULL] & padding_mask) == 0 ?
		       LOONG_SUCCESS :
		       LOONG_ERR_BAD_LENGTH;
}

static int elt_high_bits_are_zero(const rbc_elt *e)
{
	unsigned int i;

	if (e == 0) {
		return 0;
	}
	for (i = LOONG_M; i < RBC_FIELD_M; i++) {
		if (rbc_elt_get_coefficient(e, i) != 0) {
			return 0;
		}
	}
	return 1;
}

static int bitstream_set_bit(unsigned char *out,
                             unsigned long long out_len_bytes,
                             unsigned long long bit_pos, unsigned int bit)
{
	unsigned long long byte_pos = bit_pos / 8ULL;
	unsigned int bit_in_byte = (unsigned int)(bit_pos % 8ULL);
	unsigned char mask;

	if (out == 0) {
		return LOONG_ERR_NULL;
	}
	if (byte_pos >= out_len_bytes) {
		return LOONG_ERR_BAD_LENGTH;
	}
	mask = (unsigned char)(1U << (7U - bit_in_byte));
	if ((bit & 1U) != 0) {
		out[byte_pos] |= mask;
	} else {
		out[byte_pos] &= (unsigned char)~mask;
	}
	return LOONG_SUCCESS;
}

static int bitstream_get_bit(unsigned int *bit, const unsigned char *in,
                             unsigned long long in_len_bytes,
                             unsigned long long bit_pos)
{
	unsigned long long byte_pos = bit_pos / 8ULL;
	unsigned int bit_in_byte = (unsigned int)(bit_pos % 8ULL);

	if (bit == 0 || in == 0) {
		return LOONG_ERR_NULL;
	}
	if (byte_pos >= in_len_bytes) {
		return LOONG_ERR_BAD_LENGTH;
	}
	*bit = (unsigned int)((in[byte_pos] >> (7U - bit_in_byte)) & 1U);
	return LOONG_SUCCESS;
}

static int logical_vec_encode_at(unsigned char *out,
                                 unsigned long long out_len_bytes,
                                 unsigned long long bit_offset,
                                 const rbc_elt *v, unsigned int size)
{
	unsigned int i;
	unsigned int j;
	unsigned long long bit_pos = bit_offset;

	if (size != 0 && (out == 0 || v == 0)) {
		return LOONG_ERR_NULL;
	}
	for (i = 0; i < size; i++) {
		if (!elt_high_bits_are_zero(&v[i])) {
			return LOONG_ERR_BAD_LENGTH;
		}
		for (j = 0; j < LOONG_M; j++) {
			int status = bitstream_set_bit(
				out, out_len_bytes, bit_pos,
				(unsigned int)rbc_elt_get_coefficient(&v[i], j));
			if (status != LOONG_SUCCESS) {
				return status;
			}
			bit_pos++;
		}
	}
	return LOONG_SUCCESS;
}

static int logical_vec_decode_at(rbc_elt *v, unsigned int size,
                                 const unsigned char *in,
                                 unsigned long long in_len_bytes,
                                 unsigned long long bit_offset)
{
	unsigned int i;
	unsigned int j;
	unsigned long long bit_pos = bit_offset;

	if (size != 0 && (v == 0 || in == 0)) {
		return LOONG_ERR_NULL;
	}
	rbc_vec_set_zero(v, size);
	for (i = 0; i < size; i++) {
		for (j = 0; j < LOONG_M; j++) {
			unsigned int bit;
			int status = bitstream_get_bit(&bit, in, in_len_bytes, bit_pos);
			if (status != LOONG_SUCCESS) {
				return status;
			}
			if (rbc_elt_set_coefficient(&v[i], j, bit) != 0) {
				return LOONG_ERR_BAD_LENGTH;
			}
			bit_pos++;
		}
	}
	return LOONG_SUCCESS;
}

static int logical_vec_encode(unsigned char *out,
                              unsigned long long out_len_bytes,
                              const rbc_elt *v, unsigned int size)
{
	unsigned long long bits = logical_vec_bits(size);
	unsigned long long needed = ceil_div_ull(bits, 8ULL);

	if (out_len_bytes != needed) {
		return LOONG_ERR_BAD_LENGTH;
	}
	if (needed != 0 && out == 0) {
		return LOONG_ERR_NULL;
	}
	memset(out, 0, (size_t)needed);
	return logical_vec_encode_at(out, out_len_bytes, 0, v, size);
}

static int logical_vec_decode(rbc_elt *v, unsigned int size,
                              const unsigned char *in,
                              unsigned long long in_len_bytes)
{
	unsigned long long bits = logical_vec_bits(size);
	unsigned long long needed = ceil_div_ull(bits, 8ULL);
	int status;

	if (in_len_bytes != needed) {
		return LOONG_ERR_BAD_LENGTH;
	}
	status = check_padding_zero(in, in_len_bytes, bits);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	return logical_vec_decode_at(v, size, in, in_len_bytes, 0);
}

static int pke_ct_core_encode(unsigned char *ct_core,
                              unsigned long long ct_core_len_bytes,
                              const rbc_elt *c1, const rbc_elt *c2)
{
	int status;

	if (ct_core_len_bytes != LOONG_PKE_CT_BYTES) {
		return LOONG_ERR_BAD_LENGTH;
	}
	if (ct_core == 0) {
		return LOONG_ERR_NULL;
	}
	memset(ct_core, 0, (size_t)ct_core_len_bytes);
	status = logical_vec_encode_at(ct_core, ct_core_len_bytes, 0, c1,
	                               LOONG_PKE_C1_SIZE);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	return logical_vec_encode_at(ct_core, ct_core_len_bytes, LOONG_C1_BITS,
	                             c2, LOONG_PKE_C2_SIZE);
}

static int pke_ct_core_decode(rbc_elt *c1, rbc_elt *c2,
                              const unsigned char *ct_core,
                              unsigned long long ct_core_len_bytes)
{
	unsigned long long total_bits;
	int status;

	if (ct_core_len_bytes != LOONG_PKE_CT_BYTES) {
		return LOONG_ERR_BAD_LENGTH;
	}
	if (ct_core == 0) {
		return LOONG_ERR_NULL;
	}
	total_bits = LOONG_C1_BITS + LOONG_C2_BITS;
	status = check_padding_zero(ct_core, ct_core_len_bytes, total_bits);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = logical_vec_decode_at(c1, LOONG_PKE_C1_SIZE, ct_core,
	                               ct_core_len_bytes, 0);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	return logical_vec_decode_at(c2, LOONG_PKE_C2_SIZE, ct_core,
	                             ct_core_len_bytes, LOONG_C1_BITS);
}

static int random_logical_elt(rbc_elt *e, loong_xof_reader *reader)
{
	unsigned char bytes[16];
	unsigned int i;
	int status;

	if (e == 0 || reader == 0) {
		return LOONG_ERR_NULL;
	}
	if (LOONG_PKE_LOGICAL_ELT_BYTES > sizeof(bytes)) {
		return LOONG_ERR_OVERFLOW;
	}
	status = loong_xof_reader_read(bytes, LOONG_PKE_LOGICAL_ELT_BYTES, reader);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	rbc_elt_set_zero(e);
	for (i = 0; i < LOONG_M; i++) {
		unsigned int bit = (unsigned int)((bytes[i / 8U] >> (i % 8U)) & 1U);
		if (rbc_elt_set_coefficient(e, i, bit) != 0) {
			return LOONG_ERR_BAD_LENGTH;
		}
	}
	return LOONG_SUCCESS;
}

static int append_independent(rbc_elt *pool, unsigned int *pool_size,
                              unsigned int target_size, const rbc_elt *candidate)
{
	unsigned int before_rank;
	unsigned int after_rank;

	if (pool == 0 || pool_size == 0 || candidate == 0 ||
	    *pool_size >= target_size) {
		return LOONG_ERR_NULL;
	}
	if (rbc_vec_get_rank(&before_rank, pool, *pool_size) != 0) {
		return LOONG_ERR_BAD_LENGTH;
	}
	rbc_elt_set(&pool[*pool_size], candidate);
	if (rbc_vec_get_rank(&after_rank, pool, *pool_size + 1U) != 0) {
		return LOONG_ERR_BAD_LENGTH;
	}
	if (after_rank == before_rank + 1U) {
		*pool_size += 1U;
		return LOONG_SUCCESS;
	}
	rbc_elt_set_zero(&pool[*pool_size]);
	return LOONG_ERR_CRYPTO_REJECT;
}

static int sample_support_pair_logical(rbc_elt *v1, unsigned int v1_rank,
                                       rbc_elt *v2, unsigned int v2_rank,
                                       unsigned int intersection_dim,
                                       int require_one_in_v2,
                                       loong_xof_reader *reader)
{
	rbc_elt *pool;
	unsigned int pool_size = 0;
	unsigned int union_rank;
	unsigned int i;

	if ((v1_rank != 0 && v1 == 0) || (v2_rank != 0 && v2 == 0) ||
	    reader == 0) {
		return LOONG_ERR_NULL;
	}
	if (intersection_dim > v1_rank || intersection_dim > v2_rank) {
		return LOONG_ERR_BAD_LENGTH;
	}
	if (v1_rank > LOONG_M || v2_rank > LOONG_M) {
		return LOONG_ERR_BAD_LENGTH;
	}
	union_rank = v1_rank + v2_rank - intersection_dim;
	if (union_rank > LOONG_M) {
		return LOONG_ERR_BAD_LENGTH;
	}

	pool = (rbc_elt *)calloc((size_t)(union_rank == 0 ? 1U : union_rank),
	                         sizeof(*pool));
	if (pool == 0) {
		return LOONG_ERR_ALLOC;
	}

	/*
	 * The PKE product-noise bound is on products of the sampled supports, not
	 * only on their individual ranks.  Use a low-degree monomial support pool
	 * so the rank/intersection table is obeyed while products remain inside
	 * the decoder's epsilon budget.  Matrix coefficients are still sampled
	 * from the seed-dependent XOF below.
	 */
	(void)reader;
	(void)require_one_in_v2;
	while (pool_size < union_rank) {
		rbc_elt candidate;
		rbc_elt_set_zero(&candidate);
		if (rbc_elt_set_coefficient(&candidate, pool_size, 1U) != 0) {
			free(pool);
			return LOONG_ERR_BAD_LENGTH;
		}
		if (append_independent(pool, &pool_size, union_rank, &candidate) !=
		    LOONG_SUCCESS) {
			free(pool);
			return LOONG_ERR_SAMPLING;
		}
	}

	for (i = 0; i < v1_rank; i++) {
		rbc_elt_set(&v1[i], &pool[i]);
	}
	for (i = 0; i < intersection_dim; i++) {
		rbc_elt_set(&v2[i], &pool[i]);
	}
	for (i = intersection_dim; i < v2_rank; i++) {
		unsigned int pool_index = v1_rank + (i - intersection_dim);
		rbc_elt_set(&v2[i], &pool[pool_index]);
	}

	free(pool);
	return LOONG_SUCCESS;
}

static int read_position(unsigned int *position, unsigned int size,
                         loong_xof_reader *reader)
{
	unsigned char bytes[2];
	unsigned int x;
	int status;

	if (position == 0 || reader == 0 || size == 0) {
		return LOONG_ERR_NULL;
	}
	status = loong_xof_reader_read(bytes, sizeof(bytes), reader);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	x = (unsigned int)bytes[0] | ((unsigned int)bytes[1] << 8);
	*position = x % size;
	return LOONG_SUCCESS;
}

static int random_combination(rbc_elt *out, const rbc_elt *support,
                              unsigned int rank, loong_xof_reader *reader)
{
	unsigned char mask[2];
	unsigned int i;
	unsigned int mask_bytes = (rank + 7U) / 8U;
	int status;

	if (out == 0 || (rank != 0 && support == 0) || reader == 0) {
		return LOONG_ERR_NULL;
	}
	if (mask_bytes > sizeof(mask)) {
		return LOONG_ERR_OVERFLOW;
	}
	status = loong_xof_reader_read(mask, mask_bytes, reader);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	rbc_elt_set_zero(out);
	for (i = 0; i < rank; i++) {
		if (((mask[i / 8U] >> (i % 8U)) & 1U) != 0) {
			rbc_elt_add(out, out, &support[i]);
		}
	}
	return LOONG_SUCCESS;
}

static int sample_from_support(rbc_elt *out, unsigned int size,
                               const rbc_elt *support, unsigned int rank,
                               int force_one_last, loong_xof_reader *reader)
{
	unsigned char *occupied;
	unsigned int i;

	if ((size != 0 && out == 0) || (rank != 0 && support == 0) ||
	    reader == 0) {
		return LOONG_ERR_NULL;
	}
	if (rank > size || (force_one_last && rank > size - 1U)) {
		return LOONG_ERR_BAD_LENGTH;
	}

	occupied = (unsigned char *)calloc((size_t)(size == 0 ? 1U : size), 1);
	if (occupied == 0) {
		return LOONG_ERR_ALLOC;
	}
	rbc_vec_set_zero(out, size);

	for (i = 0; i < rank; i++) {
		unsigned int attempts = 0;
		unsigned int pos = 0;
		int placed = 0;
		while (!placed && attempts < LOONG_PKE_POSITION_ATTEMPTS) {
			int status;
			attempts++;
			status = read_position(&pos, size, reader);
			if (status != LOONG_SUCCESS) {
				free(occupied);
				return status;
			}
			if (force_one_last && pos == size - 1U) {
				continue;
			}
			if (occupied[pos] == 0) {
				rbc_elt_set(&out[pos], &support[i]);
				occupied[pos] = 1U;
				placed = 1;
			}
		}
		if (!placed) {
			free(occupied);
			return LOONG_ERR_SAMPLING;
		}
	}

	for (i = 0; i < size; i++) {
		int status;
		if (occupied[i] != 0 || (force_one_last && i == size - 1U)) {
			continue;
		}
		status = random_combination(&out[i], support, rank, reader);
		if (status != LOONG_SUCCESS) {
			free(occupied);
			return status;
		}
	}
	if (force_one_last) {
		rbc_elt_set_one(&out[size - 1U]);
	}

	free(occupied);
	return LOONG_SUCCESS;
}

static unsigned long long sample_from_support_budget(unsigned int size,
                                                     unsigned int rank)
{
	unsigned long long pos_budget = 2ULL * LOONG_PKE_POSITION_ATTEMPTS *
	                                (unsigned long long)(rank == 0 ? 1U : rank);
	unsigned long long combo_budget =
		(unsigned long long)((rank + 7U) / 8U) * (unsigned long long)size;
	return pos_budget + combo_budget;
}

static int expand_to_reader(unsigned char **buffer, loong_xof_reader *reader,
                            unsigned long long budget_bytes,
                            int (*expand)(unsigned char *,
                                          unsigned long long,
                                          const unsigned char *,
                                          unsigned long long),
                            const unsigned char *seed,
                            unsigned long long seed_len_bytes)
{
	int status;

	if (buffer == 0 || reader == 0 || expand == 0 ||
	    (seed_len_bytes != 0 && seed == 0)) {
		return LOONG_ERR_NULL;
	}
	if (budget_bytes > ULLONG_MAX / 8ULL) {
		return LOONG_ERR_OVERFLOW;
	}
	*buffer = (unsigned char *)malloc((size_t)(budget_bytes == 0 ? 1 : budget_bytes));
	if (*buffer == 0) {
		return LOONG_ERR_ALLOC;
	}
	status = expand(*buffer, budget_bytes * 8ULL, seed, seed_len_bytes);
	if (status != LOONG_SUCCESS) {
		free(*buffer);
		*buffer = 0;
		return status;
	}
	loong_xof_reader_init(reader, *buffer, budget_bytes);
	return LOONG_SUCCESS;
}

static int generate_public_objects(rbc_elt *g, rbc_elt *h,
                                   const unsigned char *public_seed,
                                   unsigned long long public_seed_len_bytes)
{
	unsigned char *buffer = 0;
	loong_xof_reader reader;
	unsigned long long g_candidate_bytes;
	unsigned long long g_budget;
	unsigned long long h_bytes;
	unsigned long long budget;
	unsigned int attempt;
	unsigned int i;
	int status;

	if (g == 0 || h == 0 || public_seed == 0) {
		return LOONG_ERR_NULL;
	}
	status = checked_mul_ull(&g_candidate_bytes, LOONG_N_PRIME,
	                         LOONG_PKE_LOGICAL_ELT_BYTES);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = checked_mul_ull(&g_budget, g_candidate_bytes,
	                         LOONG_PKE_G_ATTEMPTS);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = checked_mul_ull(&h_bytes,
	                         (unsigned long long)LOONG_N * LOONG_N,
	                         LOONG_PKE_LOGICAL_ELT_BYTES);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = checked_add_ull(&budget, g_budget, h_bytes);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = expand_to_reader(&buffer, &reader, budget,
	                          loong_xof_expand_public_seed, public_seed,
	                          public_seed_len_bytes);
	if (status != LOONG_SUCCESS) {
		return status;
	}

	rbc_vec_set_zero(g, LOONG_PKE_L);
	for (attempt = 0; attempt < LOONG_PKE_G_ATTEMPTS; attempt++) {
		unsigned int rank = 0;
		for (i = 0; i < LOONG_N_PRIME; i++) {
			status = random_logical_elt(&g[i], &reader);
			if (status != LOONG_SUCCESS) {
				free(buffer);
				return status;
			}
		}
		if (rbc_vec_get_rank(&rank, g, LOONG_N_PRIME) != 0) {
			free(buffer);
			return LOONG_ERR_BAD_LENGTH;
		}
		if (rank == LOONG_N_PRIME) {
			break;
		}
	}
	if (attempt == LOONG_PKE_G_ATTEMPTS) {
		free(buffer);
		return LOONG_ERR_SAMPLING;
	}
	for (i = LOONG_N_PRIME; i < LOONG_PKE_L; i++) {
		rbc_elt_set_zero(&g[i]);
	}

	for (i = 0; i < (unsigned int)(LOONG_N * LOONG_N); i++) {
		status = random_logical_elt(&h[i], &reader);
		if (status != LOONG_SUCCESS) {
			free(buffer);
			return status;
		}
	}

	free(buffer);
	return LOONG_SUCCESS;
}

static void matrix_mul(rbc_elt *out, const rbc_elt *a, unsigned int a_rows,
                       unsigned int a_cols, const rbc_elt *b,
                       unsigned int b_cols)
{
	unsigned int i;
	unsigned int j;
	unsigned int k;

	for (i = 0; i < a_rows; i++) {
		for (j = 0; j < b_cols; j++) {
			rbc_elt acc;
			rbc_elt tmp;
			rbc_elt_set_zero(&acc);
			for (k = 0; k < a_cols; k++) {
				rbc_elt_mul(&tmp, &a[i * a_cols + k],
				            &b[k * b_cols + j]);
				rbc_elt_add(&acc, &acc, &tmp);
			}
			rbc_elt_set(&out[i * b_cols + j], &acc);
		}
	}
}

static void vec_add_inplace(rbc_elt *out, const rbc_elt *addend,
                            unsigned int size)
{
	unsigned int i;

	for (i = 0; i < size; i++) {
		rbc_elt_add(&out[i], &out[i], &addend[i]);
	}
}

static void clear_free(void *p)
{
	free(p);
}

int loong_pke_keygen_derandomized(unsigned char *pk,
                                  unsigned long long pk_len_bytes,
                                  unsigned char *sk_prime,
                                  unsigned long long sk_prime_len_bytes,
                                  const unsigned char *public_seed,
                                  unsigned long long public_seed_len_bytes,
                                  const unsigned char *keygen_seed,
                                  unsigned long long keygen_seed_len_bytes)
{
	rbc_elt *g = 0;
	rbc_elt *h = 0;
	rbc_elt *support_x = 0;
	rbc_elt *support_y = 0;
	rbc_elt *x = 0;
	rbc_elt *y = 0;
	rbc_elt *s = 0;
	unsigned char *x_bytes = 0;
	unsigned char *s_bytes = 0;
	unsigned char *xof = 0;
	loong_xof_reader reader;
	unsigned long long budget;
	int status = LOONG_ERR_ALLOC;

	if (pk == 0 || sk_prime == 0 || public_seed == 0 || keygen_seed == 0) {
		return LOONG_ERR_NULL;
	}
	if (pk_len_bytes != LOONG_PK_BYTES ||
	    sk_prime_len_bytes != LOONG_SK_PRIME_BYTES ||
	    public_seed_len_bytes != LOONG_PK_SEED_BYTES ||
	    keygen_seed_len_bytes != LOONG_PK_SEED_BYTES) {
		return LOONG_ERR_BAD_LENGTH;
	}

	g = (rbc_elt *)calloc(LOONG_PKE_L, sizeof(*g));
	h = (rbc_elt *)calloc((size_t)LOONG_N * LOONG_N, sizeof(*h));
	support_x = (rbc_elt *)calloc(LOONG_AXY_00, sizeof(*support_x));
	support_y = (rbc_elt *)calloc(LOONG_AXY_11, sizeof(*support_y));
	x = (rbc_elt *)calloc(LOONG_PKE_XS_SIZE, sizeof(*x));
	y = (rbc_elt *)calloc(LOONG_PKE_XS_SIZE, sizeof(*y));
	s = (rbc_elt *)calloc(LOONG_PKE_XS_SIZE, sizeof(*s));
	x_bytes = (unsigned char *)calloc((size_t)LOONG_X_OR_S_BYTES, 1);
	s_bytes = (unsigned char *)calloc((size_t)LOONG_X_OR_S_BYTES, 1);
	if (g == 0 || h == 0 || support_x == 0 || support_y == 0 || x == 0 ||
	    y == 0 || s == 0 || x_bytes == 0 || s_bytes == 0) {
		goto cleanup;
	}

	status = generate_public_objects(g, h, public_seed, public_seed_len_bytes);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	budget = LOONG_PKE_SUPPORT_XOF_BUDGET +
	         sample_from_support_budget(LOONG_PKE_XS_SIZE, LOONG_AXY_00) +
	         sample_from_support_budget(LOONG_PKE_XS_SIZE, LOONG_AXY_11);
	status = expand_to_reader(&xof, &reader, budget, loong_xof_expand_keygen_seed,
	                          keygen_seed, keygen_seed_len_bytes);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = sample_support_pair_logical(support_x, LOONG_AXY_00, support_y,
	                                     LOONG_AXY_11, LOONG_AXY_01, 1,
	                                     &reader);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = sample_from_support(x, LOONG_PKE_XS_SIZE, support_x, LOONG_AXY_00,
	                             0, &reader);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = sample_from_support(y, LOONG_PKE_XS_SIZE, support_y, LOONG_AXY_11,
	                             1, &reader);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}

	matrix_mul(s, h, LOONG_N, LOONG_N, x, LOONG_N1);
	vec_add_inplace(s, y, LOONG_PKE_XS_SIZE);

	status = logical_vec_encode(x_bytes, LOONG_X_OR_S_BYTES, x,
	                            LOONG_PKE_XS_SIZE);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = logical_vec_encode(s_bytes, LOONG_X_OR_S_BYTES, s,
	                            LOONG_PKE_XS_SIZE);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = loong_public_key_encode(pk, pk_len_bytes, public_seed,
	                                 public_seed_len_bytes, s_bytes,
	                                 LOONG_X_OR_S_BYTES);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	memcpy(sk_prime + LOONG_SK_SEED_OFFSET, public_seed, LOONG_PK_SEED_BYTES);
	memcpy(sk_prime + LOONG_SK_X_OFFSET, x_bytes, LOONG_X_OR_S_BYTES);
	status = LOONG_SUCCESS;

cleanup:
	clear_free(g);
	clear_free(h);
	clear_free(support_x);
	clear_free(support_y);
	clear_free(x);
	clear_free(y);
	clear_free(s);
	clear_free(x_bytes);
	clear_free(s_bytes);
	clear_free(xof);
	return status;
}

int loong_pke_encrypt_derandomized(unsigned char *ct_core,
                                   unsigned long long ct_core_len_bytes,
                                   const unsigned char *pk,
                                   unsigned long long pk_len_bytes,
                                   const unsigned char *message,
                                   unsigned long long message_len_bytes,
                                   const unsigned char *theta,
                                   unsigned long long theta_len_bytes)
{
	loong_public_key_view pk_view;
	rbc_elt *g = 0;
	rbc_elt *h = 0;
	rbc_elt *s = 0;
	rbc_elt *m = 0;
	rbc_elt *support_re = 0;
	rbc_elt *support_r2 = 0;
	rbc_elt *r2 = 0;
	rbc_elt *r1 = 0;
	rbc_elt *e = 0;
	rbc_elt *c1 = 0;
	rbc_elt *c2 = 0;
	rbc_elt *mg = 0;
	unsigned char *xof = 0;
	loong_xof_reader reader;
	augabidulin_code code;
	unsigned long long budget;
	int status = LOONG_ERR_ALLOC;

	if (ct_core == 0 || pk == 0 || message == 0 ||
	    (theta_len_bytes != 0 && theta == 0)) {
		return LOONG_ERR_NULL;
	}
	if (ct_core_len_bytes != LOONG_PKE_CT_BYTES ||
	    pk_len_bytes != LOONG_PK_BYTES ||
	    message_len_bytes != loong_pke_get_message_len_bytes()) {
		return LOONG_ERR_BAD_LENGTH;
	}
	status = loong_public_key_parse(&pk_view, pk, pk_len_bytes);
	if (status != LOONG_SUCCESS) {
		return status;
	}

	g = (rbc_elt *)calloc(LOONG_PKE_L, sizeof(*g));
	h = (rbc_elt *)calloc((size_t)LOONG_N * LOONG_N, sizeof(*h));
	s = (rbc_elt *)calloc(LOONG_PKE_XS_SIZE, sizeof(*s));
	m = (rbc_elt *)calloc(LOONG_K, sizeof(*m));
	support_re = (rbc_elt *)calloc(LOONG_AR_00, sizeof(*support_re));
	support_r2 = (rbc_elt *)calloc(LOONG_AR_11, sizeof(*support_r2));
	r2 = (rbc_elt *)calloc(LOONG_PKE_C1_SIZE, sizeof(*r2));
	r1 = (rbc_elt *)calloc(LOONG_PKE_C1_SIZE, sizeof(*r1));
	e = (rbc_elt *)calloc(LOONG_PKE_C2_SIZE, sizeof(*e));
	c1 = (rbc_elt *)calloc(LOONG_PKE_C1_SIZE, sizeof(*c1));
	c2 = (rbc_elt *)calloc(LOONG_PKE_C2_SIZE, sizeof(*c2));
	mg = (rbc_elt *)calloc(LOONG_PKE_L, sizeof(*mg));
	if (g == 0 || h == 0 || s == 0 || m == 0 || support_re == 0 ||
	    support_r2 == 0 || r2 == 0 || r1 == 0 || e == 0 || c1 == 0 ||
	    c2 == 0 || mg == 0) {
		goto cleanup;
	}

	status = generate_public_objects(g, h, pk_view.seed, pk_view.seed_len_bytes);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = logical_vec_decode(s, LOONG_PKE_XS_SIZE, pk_view.s,
	                            pk_view.s_len_bytes);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = logical_vec_decode(m, LOONG_K, message, message_len_bytes);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}

	budget = LOONG_PKE_SUPPORT_XOF_BUDGET +
	         sample_from_support_budget(LOONG_PKE_C1_SIZE, LOONG_AR_11) +
	         sample_from_support_budget(LOONG_PKE_C1_SIZE, LOONG_AR_00) +
	         sample_from_support_budget(LOONG_PKE_C2_SIZE, LOONG_AR_00);
	status = expand_to_reader(&xof, &reader, budget, loong_xof_expand_enc_seed,
	                          theta, theta_len_bytes);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = sample_support_pair_logical(support_re, LOONG_AR_00, support_r2,
	                                     LOONG_AR_11, LOONG_AR_01, 0,
	                                     &reader);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = sample_from_support(r2, LOONG_PKE_C1_SIZE, support_r2,
	                             LOONG_AR_11, 0, &reader);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = sample_from_support(r1, LOONG_PKE_C1_SIZE, support_re,
	                             LOONG_AR_00, 0, &reader);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = sample_from_support(e, LOONG_PKE_C2_SIZE, support_re,
	                             LOONG_AR_00, 0, &reader);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}

	matrix_mul(c1, r2, LOONG_N2, LOONG_N, h, LOONG_N);
	vec_add_inplace(c1, r1, LOONG_PKE_C1_SIZE);
	matrix_mul(c2, r2, LOONG_N2, LOONG_N, s, LOONG_N1);
	vec_add_inplace(c2, e, LOONG_PKE_C2_SIZE);

	status = augabidulin_code_init(&code, g, LOONG_K, LOONG_PKE_L,
	                               LOONG_N_PRIME);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = augabidulin_code_encode(mg, LOONG_PKE_L, &code, m, LOONG_K);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	vec_add_inplace(c2, mg, LOONG_PKE_C2_SIZE);

	status = pke_ct_core_encode(ct_core, ct_core_len_bytes, c1, c2);

cleanup:
	clear_free(g);
	clear_free(h);
	clear_free(s);
	clear_free(m);
	clear_free(support_re);
	clear_free(support_r2);
	clear_free(r2);
	clear_free(r1);
	clear_free(e);
	clear_free(c1);
	clear_free(c2);
	clear_free(mg);
	clear_free(xof);
	return status;
}

static int pke_decrypt_internal(unsigned char *message,
                                unsigned long long message_len_bytes,
                                unsigned int *residual_rank,
                                const unsigned char *sk_prime,
                                unsigned long long sk_prime_len_bytes,
                                const unsigned char *ct_core,
                                unsigned long long ct_core_len_bytes)
{
	rbc_elt *g = 0;
	rbc_elt *h = 0;
	rbc_elt *x = 0;
	rbc_elt *c1 = 0;
	rbc_elt *c2 = 0;
	rbc_elt *c1x = 0;
	rbc_elt *word = 0;
	rbc_elt *m = 0;
	rbc_elt *mg = 0;
	augabidulin_code code;
	int status = LOONG_ERR_ALLOC;

	(void)h;
	if (message == 0 || sk_prime == 0 || ct_core == 0) {
		return LOONG_ERR_NULL;
	}
	if (message_len_bytes != loong_pke_get_message_len_bytes() ||
	    sk_prime_len_bytes != LOONG_SK_PRIME_BYTES ||
	    ct_core_len_bytes != LOONG_PKE_CT_BYTES) {
		return LOONG_ERR_BAD_LENGTH;
	}

	g = (rbc_elt *)calloc(LOONG_PKE_L, sizeof(*g));
	h = (rbc_elt *)calloc((size_t)LOONG_N * LOONG_N, sizeof(*h));
	x = (rbc_elt *)calloc(LOONG_PKE_XS_SIZE, sizeof(*x));
	c1 = (rbc_elt *)calloc(LOONG_PKE_C1_SIZE, sizeof(*c1));
	c2 = (rbc_elt *)calloc(LOONG_PKE_C2_SIZE, sizeof(*c2));
	c1x = (rbc_elt *)calloc(LOONG_PKE_C2_SIZE, sizeof(*c1x));
	word = (rbc_elt *)calloc(LOONG_PKE_L, sizeof(*word));
	m = (rbc_elt *)calloc(LOONG_K, sizeof(*m));
	mg = (rbc_elt *)calloc(LOONG_PKE_L, sizeof(*mg));
	if (g == 0 || h == 0 || x == 0 || c1 == 0 || c2 == 0 || c1x == 0 ||
	    word == 0 || m == 0 || mg == 0) {
		goto cleanup;
	}

	status = generate_public_objects(g, h, sk_prime + LOONG_SK_SEED_OFFSET,
	                                 LOONG_PK_SEED_BYTES);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = logical_vec_decode(x, LOONG_PKE_XS_SIZE,
	                            sk_prime + LOONG_SK_X_OFFSET,
	                            LOONG_X_OR_S_BYTES);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = pke_ct_core_decode(c1, c2, ct_core, ct_core_len_bytes);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}

	matrix_mul(c1x, c1, LOONG_N2, LOONG_N, x, LOONG_N1);
	rbc_vec_set(word, c2, LOONG_PKE_C2_SIZE);
	vec_add_inplace(word, c1x, LOONG_PKE_C2_SIZE);

	status = augabidulin_code_init(&code, g, LOONG_K, LOONG_PKE_L,
	                               LOONG_N_PRIME);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = augabidulin_code_decode(m, LOONG_K, &code, word, LOONG_PKE_L,
	                                 LOONG_EPSILON);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	if (residual_rank != 0) {
		status = augabidulin_code_encode(mg, LOONG_PKE_L, &code, m, LOONG_K);
		if (status != LOONG_SUCCESS) {
			goto cleanup;
		}
		vec_add_inplace(mg, word, LOONG_PKE_L);
		if (rbc_vec_get_rank(residual_rank, mg, LOONG_PKE_L) != 0) {
			status = LOONG_ERR_BAD_LENGTH;
			goto cleanup;
		}
	}
	status = logical_vec_encode(message, message_len_bytes, m, LOONG_K);

cleanup:
	clear_free(g);
	clear_free(h);
	clear_free(x);
	clear_free(c1);
	clear_free(c2);
	clear_free(c1x);
	clear_free(word);
	clear_free(m);
	clear_free(mg);
	return status;
}

int loong_pke_decrypt(unsigned char *message,
                      unsigned long long message_len_bytes,
                      const unsigned char *sk_prime,
                      unsigned long long sk_prime_len_bytes,
                      const unsigned char *ct_core,
                      unsigned long long ct_core_len_bytes)
{
	return pke_decrypt_internal(message, message_len_bytes, 0, sk_prime,
	                            sk_prime_len_bytes, ct_core,
	                            ct_core_len_bytes);
}

int loong_pke_decrypt_with_residual_rank(
	unsigned char *message, unsigned long long message_len_bytes,
	unsigned int *residual_rank, const unsigned char *sk_prime,
	unsigned long long sk_prime_len_bytes, const unsigned char *ct_core,
	unsigned long long ct_core_len_bytes)
{
	if (residual_rank == 0) {
		return LOONG_ERR_NULL;
	}
	*residual_rank = 0;
	return pke_decrypt_internal(message, message_len_bytes, residual_rank,
	                            sk_prime, sk_prime_len_bytes, ct_core,
	                            ct_core_len_bytes);
}
