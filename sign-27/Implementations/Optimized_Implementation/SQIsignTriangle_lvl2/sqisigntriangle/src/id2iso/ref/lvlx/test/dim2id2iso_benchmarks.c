#include <bench.h>
#include <bench_test_arguments.h>
#include <rng.h>
#include <id2iso.h>
#include <endomorphism_action.h>

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

// return 0 if ok, anything else if error
// sample array of odd numbers
int
id2iso_benchmarks_input_generation(quat_left_ideal_t *ideals, int bitsize, int iterations)
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
// no full test, just checks pairings
int
id2iso_basic_benchmarks_test(const ec_basis_t *input,
                             const ec_basis_t *output,
                             const ec_curve_t *domain,
                             const ec_curve_t *codomain,
                             const quat_left_ideal_t *ideal,
                             int point_order_bits)
{
    int res;
    fp2_t e1, e2, cmp;
    weil_basis(&e1, point_order_bits, input, domain);
    weil_basis(&e2, point_order_bits, output, codomain);
    int size = sizeof(digit_t) * ((ibz_bitsize(&ideal->norm) + sizeof(digit_t) * 8) / (8 * sizeof(digit_t)));
    digit_t *array;
    array = (digit_t *)malloc(size);
    memset(array, 0, size);
    ibz_to_digits(array, &(ideal->norm));
#ifndef NDEBUG
    ibz_t test;
    ibz_init(&test);
    ibz_copy_digits(&test, array, size / sizeof(digit_t));
    assert(ibz_cmp(&test, &(ideal->norm)) == 0);
    ibz_finalize(&test);
#endif
    fp2_pow_vartime(&cmp, &e1, &array[0], size / sizeof(digit_t));
    res = !fp2_is_equal(&e2, &cmp);
    free(array);
    return (res);
}

int
id2iso_dim2id2iso_benchmarks(int bitsize, int iterations, int test)
{
    int res = 0;
    int randret = 0;
    uint64_t start, end, sum;
    quat_left_ideal_t *ideals;
    ec_basis_t *output_bases;
    ec_curve_t *codomains;
    ideals = malloc(iterations * sizeof(quat_left_ideal_t));
    output_bases = malloc(iterations * sizeof(ec_basis_t));
    codomains = malloc(iterations * sizeof(ec_curve_t));
    for (int i = 0; i < iterations; i++) {
        quat_left_ideal_init(&(ideals[i]));
    }

    printf("Running dim2id2iso benchmarks for " STRINGIFY(
               SQISIGN_VARIANT) " with %d iterations, inputs of bitsize %d and the %d-bit prime\n",
           iterations,
           bitsize,
           ibz_bitsize(&(QUATALG_PINFTY.p)));
    if (test) {
        printf("Tests are run on the outputs\n");
    }

    randret = randret || id2iso_benchmarks_input_generation(ideals, bitsize, iterations);

    if (randret)
        goto fin;

    sum = 0;
    for (int iter = 0; iter < iterations; iter++) {
        start = cpucycles();
        res =
            res | !dim2id2iso_arbitrary_isogeny_evaluation(&(output_bases[iter]), &(codomains[iter]), &(ideals[iter]));
        end = cpucycles();
        sum = sum + end - start;
    }

    printf("dim2id2iso took %" PRIu64 " cycles on average per iteration\n", sum / iterations);
    if (test) {
        for (int iter = 0; iter < iterations; iter++) {
            res = res || id2iso_basic_benchmarks_test(&BASIS_EVEN,
                                                      &(output_bases[iter]),
                                                      &CURVE_E0,
                                                      &(codomains[iter]),
                                                      &(ideals[iter]),
                                                      TORSION_EVEN_POWER);
        }
    }

fin:;
    if (randret) {
        printf("Randomness failure in id2iso_dim2id2iso_benchmarks\n");
    }
    if (res) {
        printf("Tests in id2iso_dim2id2iso_benchmarks failed\n");
    }

    for (int i = 0; i < iterations; i++) {
        quat_left_ideal_finalize(&(ideals[i]));
    }
    free(ideals);
    free(output_bases);
    free(codomains);

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

    return id2iso_dim2id2iso_benchmarks(bitsize, iterations, tests);
}
