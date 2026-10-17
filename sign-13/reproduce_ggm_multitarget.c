/*
 * Reduced-width multi-target seed recovery against GreatWall's batched GGM
 * vector commitment (sign-13-3 internal nodes, sign-13-4 leaf commitments).
 *
 * Built by sign-13/reproduce_ggm_multitarget.py inside a temporary copy of the
 * reference source.  This file includes that copy's vector_com.c, so the
 * search below calls the submitted expand_chunk() (the parent-to-children map
 * of batch_vector_verify) and write_leaf() (the leaf commitment H0) unchanged.
 * The only change made to the copy is one inserted call, gw_trunc_keys(), that
 * keeps the low GW_BITS bits of every PRG key, giving a B-bit seed space while
 * leaving every output at full width.  Commitment, opening and grinding are the
 * unmodified batch_vector_commit() and batch_vector_open().
 *
 *   usage: witness tree|leaf <bits> <trials> <single-target-trials> <rng-seed>
 */
#define _POSIX_C_SOURCE 200112L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned gw_bits = 0;

static void gw_trunc_keys(void* keys, size_t n);

#include "vector_com.c"

#define LAMBDA_BYTES (SECURITY_PARAM / 8)
#define TAU BITS_PER_WITNESS
#define NODES BATCH_VECTOR_COMMIT_NODES
#define LEAVES BATCH_VECTOR_COMMIT_LEAVES

static void gw_trunc_keys(void* keys, size_t n)
{
	if (gw_bits == 0)
		return;
	unsigned char* p = (unsigned char*) keys;
	for (size_t k = 0; k < n; ++k, p += sizeof(block_secpar))
		for (size_t j = 0; j < sizeof(block_secpar); ++j)
		{
			if (8 * j >= gw_bits)
				p[j] = 0;
			else if (8 * j + 8 > gw_bits)
				p[j] &= (unsigned char) ((1u << (gw_bits - 8 * j)) - 1);
		}
}

static uint64_t rng_state;
static uint64_t rng64(void)
{
	uint64_t z = (rng_state += 0x9e3779b97f4a7c15ULL);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
	z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
	return z ^ (z >> 31);
}
static void rng_bytes(void* out, size_t n)
{
	unsigned char* p = out;
	for (size_t i = 0; i < n; ++i)
		p[i] = (unsigned char) rng64();
}

static block_secpar key_from_index(uint64_t x)
{
	block_secpar k;
	memset(&k, 0, sizeof(k));
	for (size_t j = 0; j < 8 && j < sizeof(k); ++j)
		((unsigned char*) &k)[j] = (unsigned char) (x >> (8 * j));
	gw_trunc_keys(&k, 1);
	return k;
}

/* Small open-addressing table keyed by the first 8 bytes of a target. */
typedef struct { uint64_t tag; int idx; } slot;
#define TABLE_BITS 12
#define TABLE_SIZE (1u << TABLE_BITS)
static slot table_tree[2][TABLE_SIZE];
static slot table_leaf[TABLE_SIZE];

static uint64_t tag_of(const void* p)
{
	uint64_t t;
	memcpy(&t, p, sizeof(t));
	return t;
}
static void table_clear(slot* t)
{
	for (size_t i = 0; i < TABLE_SIZE; ++i)
		t[i].idx = -1;
}
static void table_put(slot* t, const void* key, int idx)
{
	uint64_t tag = tag_of(key);
	size_t h = (size_t) (tag * 0x9e3779b97f4a7c15ULL >> (64 - TABLE_BITS));
	while (t[h].idx >= 0)
		h = (h + 1) & (TABLE_SIZE - 1);
	t[h].tag = tag;
	t[h].idx = idx;
}
/* Returns the indices of the entries whose tag matches. */
static int table_find(const slot* t, const void* key, int* out, int max)
{
	uint64_t tag = tag_of(key);
	size_t h = (size_t) (tag * 0x9e3779b97f4a7c15ULL >> (64 - TABLE_BITS));
	int n = 0;
	while (t[h].idx >= 0)
	{
		if (t[h].tag == tag && n < max)
			out[n++] = t[h].idx;
		h = (h + 1) & (TABLE_SIZE - 1);
	}
	return n;
}

/* Public view of one opening, parsed exactly as batch_vector_verify does. */
typedef struct {
	block128 iv;
	size_t hidden_pos[TAU];          /* tree position of each vector's hidden leaf */
	block_2secpar hleaf[TAU];        /* published commitment of that leaf */
	int n_rev;
	size_t rev_node[2 * SECURITY_PARAM + 64];
	block_secpar rev_seed[2 * SECURITY_PARAM + 64];
	int rev_of_node[NODES];          /* index into rev_*, or -1 */
	int hidden_leaf_vec[NODES];      /* vector index if a hidden leaf, else -1 */
} public_view;

static void parse_opening(public_view* pv, const unsigned char* opening, const uint8_t* delta)
{
	static uint8_t dont_reveal[NODES];
	memset(dont_reveal, 0, sizeof(dont_reveal));
	for (size_t i = 0; i < NODES; ++i)
	{
		pv->rev_of_node[i] = -1;
		pv->hidden_leaf_vec[i] = -1;
	}
	size_t off = 0;
	const uint8_t* d = delta;
	for (size_t v = 0; v < TAU; ++v)
	{
		unsigned int depth = v < VOLES_MAX_K ? VOLE_MAX_K : VOLE_MIN_K;
		size_t idx = 0;
		for (int j = depth - 1; j >= 0; --j)
			idx = 2 * idx + (d[j] & 1);
		d += depth;
		size_t pos = BATCH_VEC_POS_IN_TREE(v, idx);
		pv->hidden_pos[v] = pos;
		pv->hidden_leaf_vec[pos] = (int) v;
		dont_reveal[pos] = 1;
		memcpy(&pv->hleaf[v], opening + off, sizeof(block_2secpar));
		off += sizeof(block_2secpar);
	}
	pv->n_rev = 0;
	for (int i = LEAVES - 2; i >= 0; i--)
	{
		dont_reveal[i] = dont_reveal[2 * i + 1] || dont_reveal[2 * i + 2];
		if (dont_reveal[2 * i + 1] ^ dont_reveal[2 * i + 2])
		{
			size_t node = 2 * i + 1 + dont_reveal[2 * i + 1];
			pv->rev_node[pv->n_rev] = node;
			memcpy(&pv->rev_seed[pv->n_rev], opening + off, sizeof(block_secpar));
			pv->rev_of_node[node] = pv->n_rev;
			pv->n_rev++;
			off += sizeof(block_secpar);
		}
	}
}

/*
 * Public check of a candidate value for hidden node q: expand its subtree with
 * the submitted map, require every revealed descendant to be reproduced, and
 * require every hidden leaf below it to reproduce the published commitment.
 * Returns the number of hidden leaves confirmed, or -1 on any mismatch.
 */
static block_secpar stack_val[256];
static size_t stack_node[256];
static int public_check(const public_view* pv, size_t q, block_secpar value,
                        const prg_tree_fixed_key* fkt, const prg_leaf_fixed_key* fkl,
                        block_secpar* leaf_seed_out)
{
	int sp = 0, confirmed = 0;
	stack_node[sp] = q;
	stack_val[sp++] = value;
	while (sp > 0)
	{
		size_t node = stack_node[--sp];
		block_secpar val = stack_val[sp];
		int r = pv->rev_of_node[node];
		if (r >= 0)
		{
			if (memcmp(&val, &pv->rev_seed[r], sizeof(val)) != 0)
				return -1;
			continue;
		}
		if (node >= LEAVES - 1)
		{
			int v = pv->hidden_leaf_vec[node];
			if (v < 0)
				return -1;
			block_secpar sd;
			block_2secpar com;
			write_leaf(pv->iv, fkl, &val, &sd, &com);
			if (memcmp(&com, &pv->hleaf[v], sizeof(com)) != 0)
				return -1;
			if (confirmed == 0 && leaf_seed_out)
				*leaf_seed_out = sd;
			confirmed++;
			continue;
		}
		block_secpar kids[2];
		expand_chunk(0, 1, 2, pv->iv, fkt, fkl, &val, kids);
		stack_node[sp] = 2 * node + 2;
		stack_val[sp++] = kids[1];
		stack_node[sp] = 2 * node + 1;
		stack_val[sp++] = kids[0];
	}
	return confirmed;
}

static void init_keys(block128 iv, prg_tree_fixed_key* fkt, prg_leaf_fixed_key* fkl)
{
	block_secpar fixed_key_iv = block_secpar_set_zero();
	memcpy(&fixed_key_iv, &iv, sizeof(iv));
	init_fixed_keys(fkt, fkl, fixed_key_iv);
}

/* Internal-node route.  single>=0 restricts the targets to one revealed node. */
static uint64_t search_tree(const public_view* pv, block128 iv, int single, uint64_t limit,
                            size_t* hit_parent, block_secpar* hit_key, int* confirmed,
                            uint64_t* raw_hits)
{
	prg_tree_fixed_key fkt;
	prg_leaf_fixed_key fkl;
	init_keys(iv, &fkt, &fkl);
	public_view* view = (public_view*) pv;
	block128 saved_iv = view->iv;
	view->iv = iv;
	table_clear(table_tree[0]);
	table_clear(table_tree[1]);
	for (int r = 0; r < pv->n_rev; ++r)
	{
		if (single >= 0 && r != single)
			continue;
		size_t node = pv->rev_node[r];
		table_put(table_tree[(node & 1) ? 0 : 1], &pv->rev_seed[r], r);
	}
	*raw_hits = 0;
	for (uint64_t x = 0; x < limit; ++x)
	{
		block_secpar k = key_from_index(x), kids[2];
		expand_chunk(0, 1, 2, iv, &fkt, &fkl, &k, kids);
		for (int side = 0; side < 2; ++side)
		{
			int cand[4];
			int nc = table_find(table_tree[side], &kids[side], cand, 4);
			for (int c = 0; c < nc; ++c)
			{
				int r = cand[c];
				if (memcmp(&kids[side], &pv->rev_seed[r], sizeof(block_secpar)) != 0)
					continue;
				(*raw_hits)++;
				size_t node = pv->rev_node[r];
				size_t parent = (node - 1) / 2;
				size_t hidden = (node & 1) ? node + 1 : node - 1;
				int ok = public_check(pv, hidden, kids[1 - side], &fkt, &fkl, NULL);
				if (ok > 0)
				{
					*hit_parent = parent;
					*hit_key = k;
					*confirmed = ok;
					view->iv = saved_iv;
					return x + 1;
				}
			}
		}
	}
	view->iv = saved_iv;
	return 0;
}

/* Leaf-commitment route.  single>=0 restricts the targets to one vector. */
static uint64_t search_leaf(const public_view* pv, block128 iv, int single, uint64_t limit,
                            int* hit_vec, block_secpar* hit_key, block_secpar* hit_sd,
                            uint64_t* raw_hits)
{
	prg_tree_fixed_key fkt;
	prg_leaf_fixed_key fkl;
	init_keys(iv, &fkt, &fkl);
	table_clear(table_leaf);
	for (int v = 0; v < TAU; ++v)
		if (single < 0 || v == single)
			table_put(table_leaf, &pv->hleaf[v], v);
	*raw_hits = 0;
	for (uint64_t x = 0; x < limit; ++x)
	{
		block_secpar k = key_from_index(x), sd;
		block_2secpar com;
		write_leaf(iv, &fkl, &k, &sd, &com);
		int cand[4];
		int nc = table_find(table_leaf, &com, cand, 4);
		for (int c = 0; c < nc; ++c)
		{
			if (memcmp(&com, &pv->hleaf[cand[c]], sizeof(com)) != 0)
				continue;
			(*raw_hits)++;
			*hit_vec = cand[c];
			*hit_key = k;
			*hit_sd = sd;
			return x + 1;
		}
	}
	return 0;
}

int main(int argc, char** argv)
{
	if (argc != 6 || (strcmp(argv[1], "tree") && strcmp(argv[1], "leaf")))
	{
		fprintf(stderr, "usage: %s tree|leaf <bits> <trials> <single-target-trials> <rng-seed>\n", argv[0]);
		return 2;
	}
	int tree_mode = strcmp(argv[1], "tree") == 0;
	unsigned bits = (unsigned) atoi(argv[2]);
	int trials = atoi(argv[3]);
	int single_trials = atoi(argv[4]);
	rng_state = strtoull(argv[5], NULL, 0);
	if (bits < 8 || bits > 40)
		return 2;
	uint64_t space = (uint64_t) 1 << bits;

	block_secpar* tree = vector_aligned_alloc(alignof(block_secpar), NODES * sizeof(block_secpar));
	block_secpar* leaves = vector_aligned_alloc(alignof(block_secpar), LEAVES * sizeof(block_secpar));
	block_2secpar* hashed = vector_aligned_alloc(alignof(block_2secpar), LEAVES * sizeof(block_2secpar));
	unsigned char* opening = malloc(BATCH_VECTOR_OPENING_SIZE);
	public_view* pv = malloc(sizeof(*pv));
	if (!tree || !leaves || !hashed || !opening || !pv)
		return 2;

	printf("PARAMS lambda=%d tau=%d leaves=%d nodes=%d threshold=%d bits=%u\n",
	       SECURITY_PARAM, TAU, LEAVES, NODES, BATCH_VECTOR_OPENING_SEEDS_TRESHOLD, bits);

	for (int t = 0; t < trials; ++t)
	{
		block_secpar seed;
		block128 iv;
		rng_bytes(&seed, sizeof(seed));
		rng_bytes(&iv, sizeof(iv));

		gw_bits = bits;
		batch_vector_commit(seed, iv, tree, leaves, hashed);

		/* Grind the challenge exactly as force_vector_open does. */
		uint8_t delta[SECURITY_PARAM];
		int attempts = 0;
		do {
			attempts++;
			for (size_t i = 0; i < SECURITY_PARAM; ++i)
				delta[i] = i < NONZERO_BITS_IN_CHALLENGE_3 ? (uint8_t) -(rng64() & 1) : 0;
		} while (!batch_vector_open(tree, hashed, delta, opening));

		/* Everything below uses only (opening, delta, iv): the signature's public data. */
		parse_opening(pv, opening, delta);
		pv->iv = iv;

		block128 bad_iv = iv;
		((unsigned char*) &bad_iv)[0] ^= 1;

		if (tree_mode)
		{
			size_t parent = 0;
			block_secpar key;
			int conf = 0;
			uint64_t raw = 0, raw1 = 0, rawbad = 0;
			uint64_t multi = search_tree(pv, iv, -1, space, &parent, &key, &conf, &raw);
			/* Ground truth: the recovered key is the truncated true parent seed. */
			block_secpar truth = tree[parent];
			gw_trunc_keys(&truth, 1);
			int truth_ok = multi && memcmp(&truth, &key, sizeof(key)) == 0;
			size_t p1 = 0;
			block_secpar k1;
			int c1 = 0;
			uint64_t single = t < single_trials ? search_tree(pv, iv, 0, space, &p1, &k1, &c1, &raw1) : 0;
			uint64_t bad = 0;
			if (t == 0)
			{
				size_t pb;
				block_secpar kb;
				int cb;
				bad = search_tree(pv, bad_iv, -1, space, &pb, &kb, &cb, &rawbad);
			}
			printf("TRIAL %d targets=%d attempts=%d multi=%llu single=%llu truth=%d leaves_confirmed=%d"
			       " wrong_iv=%s\n",
			       t, pv->n_rev, attempts, (unsigned long long) multi, (unsigned long long) single,
			       truth_ok, conf,
			       t == 0 ? (bad ? "FOUND" : (rawbad ? "RAW" : "none")) : "skip");
		}
		else
		{
			int vec = -1, v1 = -1;
			block_secpar key, sd, k1, sd1;
			uint64_t raw = 0, raw1 = 0, rawbad = 0;
			uint64_t multi = search_leaf(pv, iv, -1, space, &vec, &key, &sd, &raw);
			int truth_ok = 0;
			if (multi)
			{
				block_secpar truth = tree[pv->hidden_pos[vec]], sd_true;
				block_2secpar com_true;
				prg_tree_fixed_key fkt;
				prg_leaf_fixed_key fkl;
				init_keys(iv, &fkt, &fkl);
				write_leaf(iv, &fkl, &truth, &sd_true, &com_true);
				gw_trunc_keys(&truth, 1);
				truth_ok = memcmp(&truth, &key, sizeof(key)) == 0 &&
				           memcmp(&sd_true, &sd, sizeof(sd)) == 0;
			}
			uint64_t single = t < single_trials ? search_leaf(pv, iv, 0, space, &v1, &k1, &sd1, &raw1) : 0;
			uint64_t bad = 0;
			if (t == 0)
			{
				int vb;
				block_secpar kb, sb;
				bad = search_leaf(pv, bad_iv, -1, space, &vb, &kb, &sb, &rawbad);
			}
			printf("TRIAL %d targets=%d attempts=%d multi=%llu single=%llu truth=%d vector=%d"
			       " wrong_iv=%s\n",
			       t, TAU, attempts, (unsigned long long) multi, (unsigned long long) single,
			       truth_ok, vec, t == 0 ? (bad ? "FOUND" : "none") : "skip");
		}
		fflush(stdout);
	}
	return 0;
}
