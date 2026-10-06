/* Recover ATLAS-512 s1 from two public (c,z) signatures with overlapping masks. */
#define _GNU_SOURCE
#include <flint/nmod_mat.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 512
#define L 7
#define UNKNOWNS (N * L)
#define Q 33554432
#define PRIME 8380417UL

static int challenge[2][N], response[2][UNKNOWNS];

static int centered(int64_t x) {
    x %= Q;
    if (x < 0) x += Q;
    return x > Q / 2 ? (int)(x - Q) : (int)x;
}

static int read_coeffs(FILE *f, const char *tag, int *dst, size_t count) {
    char *line = NULL;
    size_t cap = 0;
    if (getline(&line, &cap, f) < 0 || strncmp(line, tag, strlen(tag)) != 0) {
        free(line);
        return 0;
    }
    char *p = line + strlen(tag);
    for (size_t i = 0; i < count; i++) {
        char *end;
        dst[i] = (int)strtol(p, &end, 10);
        if (end == p) { free(line); return 0; }
        p = end;
    }
    while (isspace((unsigned char)*p)) p++;
    int okay = *p == '\0';
    free(line);
    return okay;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s public-transcript\n", argv[0]);
        return 64;
    }
    FILE *f = fopen(argv[1], "r");
    if (!f) { perror(argv[1]); return 1; }
    char *line = NULL, *pk_hex = NULL;
    size_t cap = 0;
    int n = 0, l = 0, q = 0;
    if (getline(&line, &cap, f) < 0 || sscanf(line, "PARAM %d %d %d", &n, &l, &q) != 3 ||
        n != N || l != L || q != Q) return 2;
    if (getline(&line, &cap, f) < 0 || strncmp(line, "PK ", 3) != 0) return 3;
    pk_hex = strdup(line + 3);
    free(line);
    if (!pk_hex) return 4;
    for (int s = 0; s < 2; s++)
        if (!read_coeffs(f, "C", challenge[s], N) ||
            !read_coeffs(f, "Z", response[s], UNKNOWNS)) return 5;
    if (fgetc(f) != EOF) return 6; /* No secret or hidden diagnostic input. */
    fclose(f);

    int shifts[2][L - 1], row_count = 0;
    for (int s = 0; s < 2; s++) for (int i = 0; i < L - 1; i++) {
        int found = -1;
        for (int a = 1; a < N / 2 && found < 0; a++) {
            int maximum = 0;
            for (int j = 0; j < N - a; j++) {
                int d = abs(centered((int64_t)response[s][(i + 1) * N + j] -
                                     response[s][i * N + j + a]));
                if (d > maximum) maximum = d;
                if (maximum >= 4000) break;
            }
            if (maximum < 4000) found = a;
        }
        if (found < 0) return 7;
        shifts[s][i] = found;
        row_count += N - found;
        fprintf(stderr, "signature %d, adjacent mask pair %d: shift %d\n", s + 1, i, found);
    }

    nmod_mat_t matrix, rhs, solution;
    nmod_mat_init(matrix, row_count, UNKNOWNS, PRIME);
    nmod_mat_init(rhs, row_count, 1, PRIME);
    nmod_mat_init(solution, UNKNOWNS, 1, PRIME);
    int row = 0;
    for (int s = 0; s < 2; s++) for (int i = 0; i < L - 1; i++) {
        int shift = shifts[s][i];
        for (int j = 0; j < N - shift; j++) {
            int d = centered((int64_t)response[s][(i + 1) * N + j] -
                             response[s][i * N + j + shift]);
            nmod_mat_entry(rhs, row, 0) = d >= 0 ? (ulong)d : PRIME - (ulong)(-d);
            for (int k = 0; k < N; k++) {
                int r1 = j - k;
                int r0 = j + shift - k;
                if (r1 < 0) r1 += N;
                if (r0 < 0) r0 += N;
                int a = challenge[s][r1] * (j >= k ? 1 : -1);
                int b = -challenge[s][r0] * (j + shift >= k ? 1 : -1);
                if (a) nmod_mat_entry(matrix, row, (i + 1) * N + k) = a > 0 ? 1 : PRIME - 1;
                if (b) nmod_mat_entry(matrix, row, i * N + k) = b > 0 ? 1 : PRIME - 1;
            }
            row++;
        }
    }

    long rank = (long)nmod_mat_rank(matrix);
    fprintf(stderr, "equations=%d unknowns=%d rank=%ld over GF(%lu)\n",
            row_count, UNKNOWNS, rank, PRIME);
    if (rank != UNKNOWNS || !nmod_mat_can_solve(solution, matrix, rhs)) return 8;
    for (int i = 0; i < UNKNOWNS; i++) {
        long x = (long)nmod_mat_entry(solution, i, 0);
        if (x > (long)PRIME / 2) x -= PRIME;
        if (x < -4 || x > 4) return 9;
    }
    fprintf(stderr, "recovered all %d small secret coefficients from public data\n", UNKNOWNS);
    fputs(pk_hex, stdout);
    for (int i = 0; i < UNKNOWNS; i++) {
        long x = (long)nmod_mat_entry(solution, i, 0);
        if (x > (long)PRIME / 2) x -= PRIME;
        printf("%ld%c", x, i + 1 == UNKNOWNS ? '\n' : ' ');
    }
    free(pk_hex);
    nmod_mat_clear(matrix);
    nmod_mat_clear(rhs);
    nmod_mat_clear(solution);
    return 0;
}
