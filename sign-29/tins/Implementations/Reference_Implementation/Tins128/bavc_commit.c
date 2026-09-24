#include "bavc_commit.h"
#include "auxfunc.h"
#include "ff_arith.h"


#include <memory.h>
#include <stdio.h>
#include "drng.h"


//compress aux, the procedure is only for fixed (N_TUPLE-2)%8==2;
void compress_aux(unsigned char aux[][N_TUPLE_SIZE * 2], int dim, unsigned char* msg)
{
    int stp=0;
    for(int e=0; e<dim/2; e++){
        for(int i=0; i<N_TUPLE_SIZE; i++){
            msg [stp + i] = aux[e*2][i];
        }

        msg[stp+N_TUPLE_SIZE-1] = aux[e*2][N_TUPLE_SIZE-1] ^ (aux[2*e][N_TUPLE_SIZE] >> 2);

        for(int i=0; i<N_TUPLE_SIZE-2; i++){
            msg[stp+N_TUPLE_SIZE+i] = (aux[e*2][N_TUPLE_SIZE+i]<<6) ^ (aux[2*e][N_TUPLE_SIZE+i+1] >> 2);
        }

        msg[stp+N_TUPLE_SIZE*2-2] = (aux[e*2][N_TUPLE_SIZE*2-2] << 6) ^ (aux[2*e][N_TUPLE_SIZE*2-1] >> 2)
                                    ^ (aux[2*e+1][0] >> 4);

        for(int i=0; i<N_TUPLE_SIZE-2; i++){
            msg[stp+N_TUPLE_SIZE*2-1+i] = (aux[2*e+1][i] <<4) ^ (aux[2*e+1][i+1]>>4);
        }
        msg[stp+N_TUPLE_SIZE*3-3] = (aux[2*e+1][N_TUPLE_SIZE-2]<<4) ^ (aux[2*e+1][N_TUPLE_SIZE-1]>>4) 
                                    ^ (aux[2*e+1][N_TUPLE_SIZE]>>6);

        for(int i=0; i<N_TUPLE_SIZE-2; i++){
            msg[stp+N_TUPLE_SIZE*3-2+i] = (aux[2*e+1][N_TUPLE_SIZE+i] << 2) ^ (aux[2*e+1][N_TUPLE_SIZE+i+1]>>6);
        }

        msg[stp+N_TUPLE_SIZE*4-4] = (aux[2*e+1][N_TUPLE_SIZE*2-2] << 2) ^ (aux[2*e+1][N_TUPLE_SIZE*2-1]>>6);
        stp += N_TUPLE_SIZE * 4 - 3;
    }
    if(dim&1){

        for(int i=0; i<N_TUPLE_SIZE; i++){
            msg [stp + i] = aux[dim-1][i];
        }

        msg[stp+N_TUPLE_SIZE-1] = aux[dim-1][N_TUPLE_SIZE-1] ^ (aux[dim-1][N_TUPLE_SIZE] >> 2);

        for(int i=0; i<N_TUPLE_SIZE-2; i++){
            msg[stp+N_TUPLE_SIZE+i] = (aux[dim-1][N_TUPLE_SIZE+i]<<6) ^ (aux[dim-1][N_TUPLE_SIZE+i+1] >> 2);
        }

        msg[stp+N_TUPLE_SIZE*2-2] = (aux[dim-1][N_TUPLE_SIZE*2-2] << 6) ^ (aux[dim-1][N_TUPLE_SIZE*2-1] >> 2);

        stp+=N_TUPLE_SIZE*2 - 2;
    }
}


void decompress_aux(unsigned char* msg, unsigned char aux[][N_TUPLE_SIZE * 2], int dim)
{
    int stp=0;
    for(int e=0; e<dim/2; e++){
        for(int i=0; i<N_TUPLE_SIZE-1; i++){
            aux[2*e][i] = msg [stp + i];
        }
        aux[2*e][N_TUPLE_SIZE-1] =(msg [stp + N_TUPLE_SIZE-1]>>6)<<6;

        for(int i=0; i<N_TUPLE_SIZE-1; i++){
            aux[2*e][i+N_TUPLE_SIZE] = (msg[stp+i+N_TUPLE_SIZE-1] << 2)|(msg[stp+i+N_TUPLE_SIZE]>>6);
        }

        aux[2*e][N_TUPLE_SIZE*2-1] = (msg[stp+N_TUPLE_SIZE*2-2]>>4)<<6;

        for(int i=0; i<N_TUPLE_SIZE-1; i++){
            aux[2*e+1][i] = (msg[stp+N_TUPLE_SIZE*2-2+i] << 4) | (msg[stp+N_TUPLE_SIZE*2-2+i+1]>>4);
        }
        aux[2*e+1][N_TUPLE_SIZE-1] = (msg[stp+N_TUPLE_SIZE*3-3]>>2)<<6;

        for(int i=0; i<N_TUPLE_SIZE-1; i++){
            aux[2*e+1][i+N_TUPLE_SIZE] = ((msg[stp+N_TUPLE_SIZE*3-3+i] )<<6) | (msg[stp+N_TUPLE_SIZE*3-3+i+1]>>2);
        }
        aux[2*e+1][N_TUPLE_SIZE*2-1] = ((msg[stp+N_TUPLE_SIZE*4-4] )<<6);
        stp+=N_TUPLE_SIZE*4-3;
    }


    if(dim&1){
        for(int i=0; i<N_TUPLE_SIZE-1; i++){
            aux[dim-1][i] = msg [stp + i];
        }
        aux[dim-1][N_TUPLE_SIZE-1] =(msg [stp + N_TUPLE_SIZE-1]>>6)<<6;

        for(int i=0; i<N_TUPLE_SIZE-1; i++){
            aux[dim-1][i+N_TUPLE_SIZE] = (msg[stp+i+N_TUPLE_SIZE-1] << 2)|(msg[stp+i+N_TUPLE_SIZE]>>6);
        }

        aux[dim-1][N_TUPLE_SIZE*2-1] = (msg[stp+N_TUPLE_SIZE*2-2]>>4)<<6;

        stp+=N_TUPLE_SIZE*2 - 2;
    }

}




void ChildNodeGen(unsigned char *salt, unsigned char *rseed, int idx,
                  node lchild, node rchild)
{
    unsigned char msg[RSEED_SIZE];
    memcpy(msg, salt, RSEED_SIZE);
    msg[10] ^= 0x3;
    msg[11] ^= idx & 0xff;
    msg[12] ^= (idx >> 8) & 0xff;
    msg[13] ^= (idx >> 16) & 0xff;
    msg[14] ^= (idx >> 24) & 0xff;

    // 
    pseudoXOF(NODE_SIZE * 8, msg, RSEED_SIZE * 8, lchild);
    msg[15] ^= 0x1;
    pseudoXOF(NODE_SIZE * 8, msg, RSEED_SIZE * 8, rchild);
}


void bavc_commit(unsigned char *salt, unsigned char *rseed, ggm_tree tree,
                 commitment coms[TAU][N], hash_t H_com, node seeds[TAU][N])
{

    memcpy((tree[0]), rseed, RSEED_SIZE);

    for (int i = 0; i < N_LEAVES - 1; i++)
    {
        ChildNodeGen(salt, tree[i], i, tree[2 * i + 1], tree[2 * i + 2]);
    }

    unsigned char msg[SALT_SIZE + NODE_SIZE + 5];
    msg[0] = 3;
    memcpy(msg + 1, salt, SALT_SIZE);

    for (int e = 0; e < TAU; e++)
    {
        for (int i = 0; i < N; i++)
        {
            int idx = N_LEAVES - 1 + i * TAU + e;
            memcpy((seeds[e][i]), (tree[idx]), NODE_SIZE);

            memcpy(msg + 1 + SALT_SIZE, &(seeds[e][i]), NODE_SIZE);
            memcpy(msg + 1 + SALT_SIZE + NODE_SIZE, &idx, 4);
            pseudoXOF(sizeof(hash_t)*8, msg, (SALT_SIZE + NODE_SIZE + 5) * 8, coms[e][i]);
        }
    }

    unsigned char msg1[N_LEAVES * COMMIT_SIZE + 1];
    msg1[0] = 3;
    memcpy(msg1 + 1, coms, N_LEAVES * COMMIT_SIZE);
    pseudoXOF(sizeof(hash_t)*8, msg1, N_LEAVES * COMMIT_SIZE * 8 + 8, H_com);
}

// 0 for success, -1 for failure
int bavc_open(ggm_tree tree, commitment coms[TAU][N], int challenge_poits[TAU],
              node path[T_OPEN], int *path_size, commitment proof[TAU])
{

    unsigned char revealed[2 * N_LEAVES - 1];

    memset(revealed, 0, N_LEAVES - 1);
    memset(revealed + N_LEAVES - 1, 1, N_LEAVES);

    for (int e = 0; e < TAU; e++)
    {
        int idx = N_LEAVES - 1 + challenge_poits[e] * TAU + e;
        revealed[idx] = 0;
    }

    for (int i = N_LEAVES - 2; i >= 0; i--)
    {
        if (revealed[2 * i + 1] && revealed[2 * i + 2])
        {
            revealed[i] = 1;
            revealed[2 * i + 1] = 0;
            revealed[2 * i + 2] = 0;
        }
    }

    int total_revealed = 0;
    for (int i = 0; i < N_LEAVES * 2 - 1; i++)
    {
        total_revealed += revealed[i];
    }

    if (total_revealed > T_OPEN)
    {
        return -1;
    }
    int offset = 0;
    for (int i = 0; i < 2 * N_LEAVES - 1; i++)
    {
        if (revealed[i])
        {
            memcpy((path[offset]), (tree[i]), NODE_SIZE);
            offset++;
        }
    }
    *path_size = offset;

    for (int e = 0; e < TAU; e++)
    {
        memcpy((proof[e]), (coms[e][challenge_poits[e]]), COMMIT_SIZE);
    }
    return 0;
}

void bavc_rec(int challenge_poits[TAU], node *path, commitment proof[TAU],
              unsigned char *salt, hash_t H_com, node seeds[TAU][N])
{

    unsigned char revealed[2 * N_LEAVES - 1];
    memset(revealed, 0, N_LEAVES - 1);
    memset(revealed + N_LEAVES - 1, 1, N_LEAVES);

    for (int e = 0; e < TAU; e++)
    {
        int idx = N_LEAVES - 1 + challenge_poits[e] * TAU + e;
        revealed[idx] = 0;
    }

    for (int i = N_LEAVES - 2; i >= 0; i--)
    {
        if (revealed[2 * i + 1] && revealed[2 * i + 2])
        {
            revealed[i] = 1;
            revealed[2 * i + 1] = 0;
            revealed[2 * i + 2] = 0;
        }
    }

    //ggm_tree tree;
    node *tree = calloc((2 * N_LEAVES - 1), sizeof(node));
    if (!tree) return;

    int offset = 0;
    //memset(&(tree), 0, sizeof(tree));
    for (int i = 0; i < N_LEAVES * 2 - 1; i++)
    {
        if (revealed[i])
        {
            memcpy((tree[i]), (path[offset]), NODE_SIZE);
            offset++;
        }
        if (i < N_LEAVES - 1)
        {
            unsigned char flag = 0;
            for (int j = 0; j < NODE_SIZE; j++)
            {
                flag = flag | tree[i][j];
            }
            if (flag != 0)
            {
                ChildNodeGen(salt, tree[i], i, tree[2 * i + 1], tree[2 * i + 2]);
            }
        }
    }

    //commitment coms[TAU][N];
     commitment (*coms)[N] = calloc(TAU, sizeof(commitment[N]));
    if (!coms) {
        free(tree);
        return;
    }

    for (int e = 0; e < TAU; e++)
    {
        for (int i = 0; i < N; i++)
        {
            if (i != challenge_poits[e])
            {
                unsigned char msg[(SALT_SIZE + NODE_SIZE + 5)];
                int idx = N_LEAVES - 1 + i * TAU + e;

                memcpy((seeds[e][i]), (tree[idx]), NODE_SIZE);

                msg[0] = 3;
                memcpy(msg + 1, salt, SALT_SIZE);
                memcpy(msg + 1 + SALT_SIZE, (seeds[e][i]), NODE_SIZE);
                memcpy(msg + 1 + SALT_SIZE + NODE_SIZE, &idx, 4);
                pseudoXOF(sizeof(hash_t)*8, msg, (SALT_SIZE + NODE_SIZE + 5) * 8, coms[e][i]);
            }
            else
            {
                memset((seeds[e][i]), 0, NODE_SIZE);
                memcpy((coms[e][i]), (proof[e]), COMMIT_SIZE);
            }
        }
    }

    unsigned char msg1[N_LEAVES * COMMIT_SIZE + 1];
    msg1[0] = 3;

    memcpy(msg1 + 1, coms, N_LEAVES * COMMIT_SIZE);
    pseudoXOF(sizeof(hash_t)*8, msg1, N_LEAVES * COMMIT_SIZE * 8 + 8, H_com);

    free(tree);
    free(coms);
}

void CommitPoly(unsigned char *salt, unsigned char *rseed, ggm_tree tree, commitment coms[TAU][N],
                unsigned char alpha[N_TUPLE_SIZE], unsigned char beta[N_TUPLE_SIZE],
                unsigned char aux[TAU][N_TUPLE_SIZE * 2], ff12b base[TAU][2 * N_TUPLE - 3],
                ff12b delta[TAU], hash_t h_sh)
{

    hash_t H_com;
    //node seeds[TAU][N];
    node (*seeds)[N] = calloc(TAU, sizeof(node[N]));
    if (!seeds) return;

    bavc_commit(salt, rseed, tree, coms, H_com, seeds);

    unsigned char alpha_acc[N_TUPLE_SIZE], beta_acc[N_TUPLE_SIZE], alpha_rnd[N_TUPLE_SIZE];
    unsigned char beta_rnd[N_TUPLE_SIZE];
    ff12b delta_acc, delta_base, delta_rnd, alpha_base[N_TUPLE - 2], beta_base[N_TUPLE - 2];

    unsigned char buffer[(MU + 2 * N_TUPLE - 4 + 7) / 8];

    //DRNG_ctx seed_expander[TAU][N];
    DRNG_ctx (*seed_expander)[N] = calloc(TAU, sizeof(DRNG_ctx[N]));
    if (!seed_expander) {
        free(seeds);
        return;
    }


    for (int e = 0; e < TAU; e++)
    {
        for (int i = 0; i < N; i++)
        {
            unsigned char nonce[SALT_SIZE + NODE_SIZE];
            memcpy(nonce, salt, SALT_SIZE);
            memcpy(nonce + SALT_SIZE, (seeds[e][i]), NODE_SIZE);
            init_random_number(&(seed_expander[e][i]), nonce, SALT_SIZE + NODE_SIZE);
        }
    }

    for (int e = 0; e < TAU; e++)
    {
        delta_acc = 0;
        delta_base = 0;
        memset(alpha_acc, 0, N_TUPLE_SIZE);
        memset(beta_acc, 0, N_TUPLE_SIZE);
        memset(alpha_base, 0, sizeof(alpha_base));
        memset(beta_base, 0, sizeof(beta_base));

        for (int i = 0; i < N; i++)
        {
            get_random_number(&(seed_expander[e][i]), buffer, MU + 2 * N_TUPLE - 4);
            memcpy(alpha_rnd, buffer, N_TUPLE_SIZE);
            memcpy(beta_rnd, buffer + N_TUPLE_SIZE, N_TUPLE_SIZE);

            ff12b t = (alpha_rnd[N_TUPLE_SIZE - 1] & 0x3f);
            delta_rnd = (t << 6) | (beta_rnd[N_TUPLE_SIZE - 1] & 0x3f);
            alpha_rnd[N_TUPLE_SIZE - 1] = alpha_rnd[N_TUPLE_SIZE - 1] & 0xc0;
            beta_rnd[N_TUPLE_SIZE - 1] = beta_rnd[N_TUPLE_SIZE - 1] & 0xc0;


            for (int j = 0; j < N_TUPLE_SIZE; j++)
            {
                alpha_acc[j] ^= alpha_rnd[j];
                beta_acc[j] ^= beta_rnd[j];
            }
            delta_acc ^= delta_rnd;

            ff12b phi = i;

            ff12b_cond_add(alpha_base, alpha_rnd, N_TUPLE - 2, phi);
            ff12b_cond_add(beta_base, beta_rnd, N_TUPLE - 2, phi);

            ff12b tmp = 0;
            ff12b_mul(i, delta_rnd, &tmp);
            ff12b_add(delta_base, tmp, &delta_base);
        }

        for (int j = 0; j < N_TUPLE_SIZE; j++)
        {
            aux[e][j] = alpha[j] ^ alpha_acc[j];
            aux[e][j + N_TUPLE_SIZE] = beta[j] ^ beta_acc[j];
        }

        for (int j = 0; j < N_TUPLE - 2; j++)
        {
            base[e][j] = alpha_base[j];
            base[e][j + N_TUPLE - 2] = beta_base[j];
        }
        base[e][2 * N_TUPLE - 4] = delta_base;
        delta[e] = delta_acc;
    }

    unsigned char msg[1 + SALT_SIZE + sizeof(H_com) + (TAU * (N_TUPLE-2) * 2+7)/8];
    msg[0] = 1;
    memcpy(msg + 1, salt, SALT_SIZE);
    memcpy(msg + 1 + SALT_SIZE, H_com, sizeof(H_com));



    // memcpy(msg + 1 + SALT_SIZE + sizeof(H_com), aux, TAU * N_TUPLE_SIZE * 2);
    int stp = 1 + SALT_SIZE + sizeof(H_com);
    memset(msg+stp, 0, (TAU * (N_TUPLE-2) * 2+7)/8);
    compress_aux(aux, TAU, msg+stp);

    

    pseudoXOF(sizeof(hash_t)*8, msg, (1 + SALT_SIZE + sizeof(H_com))*8 + (N_TUPLE-2)*2*TAU, h_sh);
    
    free(seeds);
    free(seed_expander);

}

void ComputePoly(unsigned char alpha[N_TUPLE_SIZE], unsigned char beta[N_TUPLE_SIZE],
                 ff12b base[2 * N_TUPLE - 3], ff12b delta_acc, fe u[N_TUPLE], fe v[N_TUPLE], fe p_mid,
                 fe p_base)
{
    ff12b alpha_base[N_TUPLE - 2], beta_base[N_TUPLE - 2], delta_base;

    for (int i = 0; i < N_TUPLE - 2; i++)
    {
        alpha_base[i] = base[i];
        beta_base[i] = base[i + N_TUPLE - 2];
    }
    delta_base = base[2 * N_TUPLE - 4];

    fe tmp, sum[8];

    // sum[0] = (u, beta)+u[n-1];
    fe_f2_inner(u, beta, N_TUPLE - 2, sum[0]);
    fe_add(sum[0], u[N_TUPLE - 1], sum[0]);

    // sum[1] = (v, alpha)+v[n-2];
    fe_f2_inner(v, alpha, N_TUPLE - 2, sum[1]);
    fe_add(sum[1], v[N_TUPLE - 2], sum[1]);

    // sum[2] = (v, beta)+v[n-1];
    fe_f2_inner(v, beta, N_TUPLE - 2, sum[2]);
    fe_add(sum[2], v[N_TUPLE - 1], sum[2]);

    // sum[3] = (u, alpha)+u[n-2];
    fe_f2_inner(u, alpha, N_TUPLE - 2, sum[3]);
    fe_add(sum[3], u[N_TUPLE - 2], sum[3]);

    // sum[4] = (v, beta_base);
    fe_ff12b_inner(v, beta_base, N_TUPLE - 2, sum[4]);
    // sum[5] = (v, alpha_base);
    fe_ff12b_inner(v, alpha_base, N_TUPLE - 2, sum[5]);
    // sum[6] = (u, alpha_base);
    fe_ff12b_inner(u, alpha_base, N_TUPLE - 2, sum[6]);
    // sum[7] = (u, beta_base);
    fe_ff12b_inner(u, beta_base, N_TUPLE - 2, sum[7]);

    fe_mul(sum[3], sum[4], p_mid);
    fe_mul(sum[2], sum[6], tmp);
    fe_add(p_mid, tmp, p_mid);

    fe_mul(sum[0], sum[5], tmp);
    fe_add(p_mid, tmp, p_mid);

    fe_mul(sum[7], sum[1], tmp);

    fe_add(p_mid, tmp, p_mid);

    p_mid[EXT_DEGREE - 1] = p_mid[EXT_DEGREE - 1] ^ delta_acc;

    fe_mul(sum[6], sum[4], p_base);
    fe_mul(sum[7], sum[5], tmp);
    fe_add(p_base, tmp, p_base);

    p_base[EXT_DEGREE - 1] = p_base[EXT_DEGREE - 1] ^ delta_base;
}


void ExpandChallengePoint(int challenge_points[TAU], unsigned char *v_grinding,
                          unsigned char seed[2 * LAMBDA / 8], long long ctr)
{
    int u = 12;

    DRNG_ctx cp_expander;
    unsigned char nonce[2 * LAMBDA / 8 + sizeof(long long)], buffer[(TAU * u + Omega + 7) / 8];
    memcpy(nonce, seed, 2 * LAMBDA / 8);
    memcpy(nonce + 2 * LAMBDA / 8, (unsigned char *)&ctr, sizeof(long long));

    init_random_number(&cp_expander, nonce, 2 * LAMBDA / 8 + sizeof(long long));
    get_random_number(&cp_expander, buffer, TAU * MU + Omega);

    for (int i = 0; i * 2 < TAU; i++)
    {
        challenge_points[i * 2] = buffer[i * 3];
        challenge_points[i * 2] = (challenge_points[i * 2] << 4) | (buffer[i * 3 + 1] >> 4);
        challenge_points[i * 2 + 1] = buffer[i * 3 + 1] & 0xf;
        challenge_points[i * 2 + 1] = (challenge_points[i * 2 + 1] << 8) | (buffer[i * 3 + 2]);
    }
    if (TAU & 1)
    {
        challenge_points[TAU - 1] = buffer[(TAU - 1) / 2 * 3];
        challenge_points[TAU - 1] = (challenge_points[TAU - 1] << 4) | (buffer[(TAU - 1) / 2 * 3 + 1] >> 4);
    }

    *v_grinding = (buffer[(TAU * u + Omega + 7) / 8 - 1] >> 4) | (buffer[(TAU - 1) / 2 * 3 + 1] << 4);
}


void OpenRandomEva(ggm_tree tree, commitment coms[TAU][N], hash_t h_piop, long long *ctr, node path[T_OPEN], int *path_size, commitment proof[TAU])
{
    *ctr = 0;
    unsigned char v_grinding;
    int challenge_points[TAU];
    while (1) {
        ExpandChallengePoint(challenge_points, &v_grinding, h_piop, *ctr);
        
        // 只有当 (v_grinding == 0) 时，才执行耗时的 bavc_open
        if (v_grinding == 0) {
            if (bavc_open(tree, coms, challenge_points, path, path_size, proof) == 0) {
                break; 
            }
            
        }
        
        (*ctr)++;
    }
    //fprintf(stderr, "grinding ctr = %lld\n", *ctr); fflush(stderr);
}

// 0 for success, -1 for failure;
int ComputeEva(unsigned char salt[SALT_SIZE], long long ctr, hash_t h_piop, node path[T_OPEN],
               int path_size, commitment proof[TAU], unsigned char aux[TAU][N_TUPLE_SIZE * 2],
               unsigned char *v_grinding, int points[TAU], ff12b evals[TAU][2 * N_TUPLE - 3], hash_t h_sh)
{

    int challenge_points[TAU];
    ExpandChallengePoint(challenge_points, v_grinding, h_piop, ctr);
    if (*v_grinding != 0)
    {
        return -1;
    }

    hash_t H_com;
    //node seeds[TAU][N];
    node (*seeds)[N] = calloc(TAU, sizeof(node[N]));
    if (!seeds) return -1;

    bavc_rec(challenge_points, path, proof, salt, H_com, seeds);

    unsigned char msg[1 + SALT_SIZE + sizeof(H_com) + (TAU * (N_TUPLE-2) * 2+7)/8];
    msg[0] = 1;
    memcpy(msg + 1, salt, SALT_SIZE);
    memcpy(msg + 1 + SALT_SIZE, H_com, sizeof(H_com));

    int stp = 1 + SALT_SIZE + sizeof(H_com);
    memset(msg+stp, 0, (TAU * (N_TUPLE-2) * 2+7)/8);
    compress_aux(aux, TAU, msg+stp);

    pseudoXOF(sizeof(hash_t)*8, msg, (1 + SALT_SIZE + sizeof(H_com))*8 + (N_TUPLE-2)*2*TAU, h_sh);

    unsigned char alpha_rnd[N_TUPLE_SIZE], beta_rnd[N_TUPLE_SIZE];
    unsigned char alpha_aux[N_TUPLE_SIZE], beta_aux[N_TUPLE_SIZE];
    ff12b delta_eval, delta_rnd, alpha_eval[N_TUPLE - 2], beta_eval[N_TUPLE - 2];

    //DRNG_ctx seed_expander[TAU][N];
     DRNG_ctx (*seed_expander)[N] = calloc(TAU, sizeof(DRNG_ctx[N]));
    if (!seed_expander) {
        free(seeds);
        return -1;
    }


    for (int e = 0; e < TAU; e++)
    {
        for (int i = 0; i < N; i++)
        {
            unsigned char nonce[SALT_SIZE + NODE_SIZE];
            memcpy(nonce, salt, SALT_SIZE);
            memcpy(nonce + SALT_SIZE, seeds[e][i], NODE_SIZE);
            init_random_number(&(seed_expander[e][i]), nonce, SALT_SIZE + NODE_SIZE);
        }
    }


    unsigned char buffer[(MU + 2 * N_TUPLE - 4 + 7) / 8];


    for (int e = 0; e < TAU; e++)
    {
        delta_eval = 0;
        memset(alpha_eval, 0, sizeof(alpha_eval));
        memset(beta_eval, 0, sizeof(alpha_eval));

        for (int i = 0; i < N; i++)
        {
            if (i != challenge_points[e])
            {
                get_random_number(&(seed_expander[e][i]), buffer, MU + 2 * N_TUPLE - 4);
                memcpy(alpha_rnd, buffer, N_TUPLE_SIZE);
                memcpy(beta_rnd, buffer + N_TUPLE_SIZE, N_TUPLE_SIZE);

                ff12b t = (alpha_rnd[N_TUPLE_SIZE - 1] & 0x3f);
                delta_rnd = (t << 6) | (beta_rnd[N_TUPLE_SIZE - 1] & 0x3f);
                alpha_rnd[N_TUPLE_SIZE - 1] = alpha_rnd[N_TUPLE_SIZE - 1] & 0xc0;
                beta_rnd[N_TUPLE_SIZE - 1] = beta_rnd[N_TUPLE_SIZE - 1] & 0xc0;

                ff12b a;
                ff12b_add(i, challenge_points[e], &a);

                ff12b tmp = 0;
                ff12b_mul(a, delta_rnd, &tmp);

                ff12b_add(delta_eval, tmp, &delta_eval);

                ff12b_cond_add(alpha_eval, alpha_rnd, N_TUPLE - 2, a);
                ff12b_cond_add(beta_eval, beta_rnd, N_TUPLE - 2, a);
            }
        }

        points[e] = challenge_points[e];
        ff12b tmp = points[e];

        memcpy(alpha_aux, &(aux[e]), N_TUPLE_SIZE);
        memcpy(beta_aux, &(aux[e][N_TUPLE_SIZE]), N_TUPLE_SIZE);

        ff12b_cond_add(alpha_eval, alpha_aux, N_TUPLE - 2, tmp);
        ff12b_cond_add(beta_eval, beta_aux, N_TUPLE - 2, tmp);

        for (int j = 0; j < N_TUPLE - 2; j++)
        {
            evals[e][j] = alpha_eval[j];
            evals[e][j + N_TUPLE - 2] = beta_eval[j];
        }
        evals[e][2 * N_TUPLE - 4] = delta_eval;
    }

    free(seeds);
    free(seed_expander);
    return 0;
}

void RecomputePolyProof(int points, ff12b evals[2 * N_TUPLE - 3], fe v[N_TUPLE],
                        fe u[N_TUPLE], fe p_mid, fe p_base)
{

    ff12b delta_eval, delta_rnd, alpha_eval[N_TUPLE - 2], beta_eval[N_TUPLE - 2];
    fe p_eval;

    for (int j = 0; j < N_TUPLE - 2; j++)
    {
        alpha_eval[j] = evals[j];
        beta_eval[j] = evals[j + N_TUPLE - 2];
    }
    delta_eval = evals[2 * N_TUPLE - 4];

    fe tmp;
    fe sum[4];
    ff12b phi = points;

    // sum[0]=(u, alpha_eval)+u_(n-2) * i
    fe_ff12b_inner(u, alpha_eval, N_TUPLE - 2, sum[0]);
    fe_ff12b_mul(u[N_TUPLE - 2], phi, tmp);
    fe_add(sum[0], tmp, sum[0]);

    // sum[1]=(v, beta_eval)+v_(n-1) * i
    fe_ff12b_inner(v, beta_eval, N_TUPLE - 2, sum[1]);
    fe_ff12b_mul(v[N_TUPLE - 1], phi, tmp);
    fe_add(sum[1], tmp, sum[1]);

    // sum[2]=(u, beta_eval)+u_(n-1) * i
    fe_ff12b_inner(u, beta_eval, N_TUPLE - 2, sum[2]);
    fe_ff12b_mul(u[N_TUPLE - 1], phi, tmp);
    fe_add(sum[2], tmp, sum[2]);

    // sum[3]=(v, alpha_eval)+v_(n-2) * i
    fe_ff12b_inner(v, alpha_eval, N_TUPLE - 2, sum[3]);
    fe_ff12b_mul(v[N_TUPLE - 2], phi, tmp);
    fe_add(sum[3], tmp, sum[3]);

    fe_mul(sum[0], sum[1], p_eval);
    fe_mul(sum[2], sum[3], tmp);
    fe_add(p_eval, tmp, p_eval);

    p_eval[EXT_DEGREE - 1] = p_eval[EXT_DEGREE - 1] ^ delta_eval;

    // p_base =p_eval - p_mid * point
    fe_ff12b_mul(p_mid, phi, tmp);
    fe_add(p_eval, tmp, p_base);
}
