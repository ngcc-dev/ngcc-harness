#ifndef BAVC_COMMIT_H
#define BAVC_COMMIT_H
#include "params.h"
#include "ff_arith.h"

typedef unsigned char commitment[COMMIT_SIZE];
typedef unsigned char node[NODE_SIZE];
typedef node ggm_tree[2 * N_LEAVES - 1];

typedef unsigned char hash_t[COMMIT_SIZE];


void compress_aux(unsigned char aux[][N_TUPLE_SIZE * 2], int dim, unsigned char* nonce);

void decompress_aux(unsigned char* nonce, unsigned char aux[][N_TUPLE_SIZE * 2], int dim);

void bavc_commit(unsigned char *salt, unsigned char *rseed, ggm_tree tree,
                 commitment coms[TAU][N], hash_t H_com, node seeds[TAU][N]);

int bavc_open(ggm_tree tree, commitment coms[TAU][N], int challenge_poits[TAU],
              node path[T_OPEN], int *path_size, commitment proof[TAU]);

void bavc_rec(int *challenge_poits, node *path, commitment proof[TAU],
              unsigned char *salt, hash_t H_com, node seeds[TAU][N]);

void CommitPoly(unsigned char *salt, unsigned char *rseed, ggm_tree tree, commitment coms[TAU][N],
                unsigned char alpha[N_TUPLE_SIZE], unsigned char beta[N_TUPLE_SIZE],
                unsigned char aux[TAU][N_TUPLE_SIZE * 2], ff12b base[TAU][2 * N_TUPLE - 3],
                ff12b delta[TAU], hash_t h_sh);

void ComputePoly(unsigned char alpha[N_TUPLE_SIZE], unsigned char beta[N_TUPLE_SIZE],
                 ff12b base[2 * N_TUPLE - 3], ff12b delta_acc, fe u[N_TUPLE], fe v[N_TUPLE], fe p_mid,
                 fe p_base);

void ExpandChallengePoint(int challenge_points[TAU], unsigned char *v_grinding,
                          unsigned char seed[LAMBDA / 8], long long ctr);

void OpenRandomEva(ggm_tree tree, commitment coms[TAU][N], hash_t h_piop, long long *ctr,
                   node path[T_OPEN], int *path_size, commitment proof[TAU]);

int ComputeEva(unsigned char salt[SALT_SIZE], long long ctr, hash_t h_piop, node path[T_OPEN],
               int path_size, commitment proof[TAU], unsigned char aux[TAU][N_TUPLE_SIZE * 2],
               unsigned char *v_grinding, int points[TAU], ff12b evals[TAU][2 * N_TUPLE - 3], hash_t h_sh);
void RecomputePolyProof(int points, ff12b evals[2 * N_TUPLE - 3], fe v[N_TUPLE],
                        fe u[N_TUPLE], fe p_mid, fe p_base);

#endif