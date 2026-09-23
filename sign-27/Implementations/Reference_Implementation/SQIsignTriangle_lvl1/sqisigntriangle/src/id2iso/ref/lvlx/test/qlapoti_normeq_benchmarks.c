#include <bench.h>
#include <bench_test_arguments.h>
#include <rng.h>
#include <id2iso.h>

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

// return 0 if ok, anything else if error
// sample array of odd numbers
int
id2iso_qlapoti_benchmarks_input_generation(quat_left_ideal_t *ideals, int bitsize, int iterations)
{
    int randret = 1;
    ibz_t norm;
    ibz_init(&norm);
    for (int i = 0; i < iterations; i++) {
        randret = randret && ibz_generate_random_prime(&(norm), 0, bitsize, QUAT_primality_num_iter);
        if (!randret)
            goto fin;
        randret = randret && quat_sampling_random_ideal_O0_given_norm(
                                 &(ideals[i]), &norm, 1, &QUAT_represent_integer_params, &QUAT_prime_cofactor);

        if (!randret)
            goto fin;
    }
fin:;
    ibz_finalize(&norm);
    return (!randret);
}

// 0 if ok, 1 otherwise
int
id2iso_qlapoti_benchmarks_test(const quat_alg_elem_t *mu1,
                               const quat_alg_elem_t *mu2,
                               const quat_alg_elem_t *theta,
                               const quat_alg_elem_t *small,
                               const quat_left_ideal_t *lideal,
                               int two_power)
{
    ibz_t n, cofactor;
    quat_alg_elem_t mu1c, mu2c, smallc, cmp;
    quat_left_ideal_t small_equiv, I1, I2;
    ibz_init(&n);
    ibz_init(&cofactor);
    quat_left_ideal_init(&small_equiv);
    quat_left_ideal_init(&I1);
    quat_left_ideal_init(&I2);
    quat_alg_elem_init(&mu1c);
    quat_alg_elem_init(&mu2c);
    quat_alg_elem_init(&smallc);
    quat_alg_elem_init(&cmp);

    int res = 0;

    res = res || !quat_lattice_contains(NULL, &lideal->lattice, small);
    quat_alg_conj(&smallc, small);
    ibz_mul(&smallc.denom, &smallc.denom, &lideal->norm);
    quat_lideal_mul(&small_equiv, lideal, &smallc, &QUATALG_PINFTY);

    res = res || !quat_lattice_contains(NULL, &small_equiv.lattice, mu1);
    res = res || !quat_lattice_contains(NULL, &small_equiv.lattice, mu2);
    ibz_sqrt_floor(&n, &QUATALG_PINFTY.p);
    res = res || !(ibz_cmp(&small_equiv.norm, &n) < 0);
    quat_alg_conj(&mu1c, mu1);
    ibz_mul(&mu1c.denom, &mu1c.denom, &small_equiv.norm);
    quat_lideal_mul(&I1, &small_equiv, &mu1c, &QUATALG_PINFTY);
    quat_alg_mul(&cmp, mu2, &mu1c, &QUATALG_PINFTY);
    res = res || !quat_alg_elem_equal(&cmp, theta);
    quat_alg_conj(&mu2c, mu2);
    ibz_mul(&mu2c.denom, &mu2c.denom, &small_equiv.norm);
    quat_lideal_mul(&I2, &small_equiv, &mu2c, &QUATALG_PINFTY);
    ibz_add(&n, &I1.norm, &I2.norm);
    ibz_pow(&cofactor, &ibz_const_two, two_power);
    res = res || !(ibz_cmp(&cofactor, &n) == 0);

    ibz_finalize(&n);
    ibz_finalize(&cofactor);
    quat_left_ideal_finalize(&small_equiv);
    quat_left_ideal_finalize(&I1);
    quat_left_ideal_finalize(&I2);
    quat_alg_elem_finalize(&mu1c);
    quat_alg_elem_finalize(&mu2c);
    quat_alg_elem_finalize(&smallc);
    quat_alg_elem_finalize(&cmp);

    return (res);
}

int
id2iso_qlapoti_benchmarks(int bitsize, int iterations, int test)
{
    int res = 0;
    int randret = 0;
    uint64_t start, end, sum;
    quat_left_ideal_t *ideals;
    quat_alg_elem_t *mu1s, *mu2s, *thetas, *smalls;
    ideals = malloc(iterations * sizeof(quat_left_ideal_t));
    mu1s = malloc(iterations * sizeof(quat_alg_elem_t));
    mu2s = malloc(iterations * sizeof(quat_alg_elem_t));
    thetas = malloc(iterations * sizeof(quat_alg_elem_t));
    smalls = malloc(iterations * sizeof(quat_alg_elem_t));
    for (int i = 0; i < iterations; i++) {
        quat_left_ideal_init(&(ideals[i]));
        quat_alg_elem_init(&(mu1s[i]));
        quat_alg_elem_init(&(mu2s[i]));
        quat_alg_elem_init(&(thetas[i]));
        quat_alg_elem_init(&(smalls[i]));
    }

    printf("Running qlapoti_normeq benchmarks for " STRINGIFY(
               SQISIGN_VARIANT) " with %d iterations, inputs of bitsize %d and the %d-bit prime\n",
           iterations,
           bitsize,
           ibz_bitsize(&(QUATALG_PINFTY.p)));
    if (test) {
        printf("Tests are run on the outputs\n");
    }

    randret = randret || id2iso_qlapoti_benchmarks_input_generation(ideals, bitsize, iterations);

    if (randret)
        goto fin;

    sum = 0;
    for (int iter = 0; iter < iterations; iter++) {
        start = cpucycles();
        res = res | !quat_qlapoti(&(mu1s[iter]),
                                  &(mu2s[iter]),
                                  &(thetas[iter]),
                                  &(smalls[iter]),
                                  &(ideals[iter]),
                                  &QUATALG_PINFTY,
                                  QUAT_qlapoti_iteration_bound,
                                  QUAT_qlapoti_gen_bound_bits,
                                  QUAT_qlapoti_used_power_of_two,
                                  &QUAT_cornacchia_extended_params);
        end = cpucycles();
        sum = sum + end - start;
    }

    printf("QLapoti took %" PRIu64 " cycles on average per iteration\n", sum / iterations);
    if (test) {
        for (int iter = 0; iter < iterations; iter++) {
            res = res || id2iso_qlapoti_benchmarks_test(&(mu1s[iter]),
                                                        &(mu2s[iter]),
                                                        &(thetas[iter]),
                                                        &(smalls[iter]),
                                                        &(ideals[iter]),
                                                        QUAT_qlapoti_used_power_of_two);
        }
    }

fin:;
    if (randret) {
        printf("Randomness failure in id2iso_qlapoti_normeq_benchmarks\n");
    }
    if (res) {
        printf("Tests in id2iso_qlapoti_normeq_benchmarks failed\n");
    }

    for (int i = 0; i < iterations; i++) {
        quat_left_ideal_finalize(&(ideals[i]));
        quat_alg_elem_finalize(&(mu1s[i]));
        quat_alg_elem_finalize(&(mu2s[i]));
        quat_alg_elem_finalize(&(thetas[i]));
        quat_alg_elem_finalize(&(smalls[i]));
    }
    free(mu1s);
    free(mu2s);
    free(smalls);
    free(thetas);
    free(ideals);

    return (res | 2 * randret);
}

int
main(int argc, char *argv[])
{
    uint32_t seed[12];
    int iterations = 10 * SQISIGN_TEST_REPS;
    int help = 0;
    int seed_set = 0;
    int tests = 0;
    int bitsize = ibz_bitsize(&(QUATALG_PINFTY.p)) * 2;
    int invalid = 0;

#ifndef NDEBUG
    fprintf(stderr,
            "\x1b[31mIt looks like SQIsign was compiled with assertions enabled.\n"
            "This will severely impact performance measurements.\x1b[0m\n");
#endif

    for (int i = 1; i < argc; i++) {
        if (!tests && strcmp(argv[i], "--tests") == 0) {
            tests = 1;
            continue;
        }

        if (!seed_set && !parse_seed(argv[i], seed)) {
            seed_set = 1;
            continue;
        }

        if (!help && strcmp(argv[i], "--help") == 0) {
            help = 1;
            continue;
        }

        if (sscanf(argv[i], "--iterations=%d", &iterations) == 1) {
            continue;
        }

        if (sscanf(argv[i], "--bitsize=%d", &bitsize) == 1) {
            continue;
        }
    }

    invalid = invalid || (argc > 4);
    invalid = invalid || (iterations < 0);
    invalid = invalid || (bitsize < 1);

    if (help || invalid) {
        if (invalid) {
            printf("Invalid input\n");
        }
        printf("Usage: %s [--bitsize=<bitsize>] [--iterations=<iterations>] [--tests] [--seed=<seed>]\n", argv[0]);
        printf("Where <bitsize> is the bitsize of the norm of the O0-ideals used as inputs for benchmarking. It must "
               "be at least 1; if no present, uses the default %d;\n",
               bitsize);
        printf("Where <iterations> is the number of iterations used for benchmarking; if not present, uses the "
               "default: %d)\n",
               iterations);
        printf("Where <seed> is the random seed to be used; if not present, a random seed is generated\n");
        printf("Additional verifications are run on each output if --tests is passed\n");
        printf("Output has last bit set if tests failed, second-to-last if randomness failed\n");
        return 1;
    }

    if (!seed_set) {
        randombytes_select((unsigned char *)seed, sizeof(seed));
    }

    print_seed(seed);

#if defined(TARGET_BIG_ENDIAN)
    for (int i = 0; i < 12; i++) {
        seed[i] = BSWAP32(seed[i]);
    }
#endif

    randombytes_init((unsigned char *)seed, NULL, 256);
    cpucycles_init();

    return id2iso_qlapoti_benchmarks(bitsize, iterations, tests);
}
