#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <inttypes.h>

#include "biextension.h"
#include "curve_extras.h"
#include <endomorphism_action.h>
#include <torsion_constants.h>
#include <tools.h>

static inline int64_t
cpucycles(void)
{
#if (defined(TARGET_AMD64) || defined(TARGET_X86))
    unsigned int hi, lo;

    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((int64_t)lo) | (((int64_t)hi) << 32);
#elif (defined(TARGET_S390X))
    uint64_t tod;
    asm volatile("stckf %0\n" : "=Q"(tod) : : "cc");
    return (tod * 1000 / 4096);
#else
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    return (int64_t)(time.tv_sec * 1e9 + time.tv_nsec);
#endif
}

static __inline__ uint64_t
rdtsc(void)
{
    return (uint64_t)cpucycles();
}

void
fp2_exp_2e(fp2_t *r, uint64_t e, fp2_t const *x)
{
    fp2_copy(r, x);
    for (uint64_t i = 0; i < e; i++) {
        fp2_sqr(r, r);
    }
}

void
dbl_2e(ec_point_t *R, uint64_t e, ec_point_t const *P, ec_point_t const *A24)
{
    copy_point(R, P);
    for (uint64_t i = 0; i < e; i++) {
        xDBL_A24(R, R, A24);
    }
}

/*
void cubical_2e(ec_point_t* R, uint64_t e, ec_point_t const* P, ec_point_t const* A24)
{
    copy_point(R, P);
    for (uint64_t i=0; i<e; i++) {
        cubicalDBL(R, R, A24);
    }
}
*/
int
biextension_test(uint64_t bench)
{
    clock_t t;
    uint64_t t0, t1;
    float ms;
    ec_curve_t E0 = CURVE_E0;
    uint64_t e = TORSION_PLUS_EVEN_POWER;
    // ibz_t two_pow, tmp;
    fp2_t one, r1, rr1, rrr1, r2, r3;
    ec_point_t P, Q, PmQ, A24, AC;
    ec_point_t tmp, PQ, PP, QQ, PPQ, PQQ, PPP, QQQ, PPPQ, PQQQ;

    // ibz_init(&two_pow); ibz_init(&tmp);
    // ibz_pow(&two_pow,&ibz_const_two,length);

    copy_point(&AC, &CURVE_E0_A24); // Warning, this is AC, not A24!
    A24_from_AC(&A24, &AC);
    copy_point(&P, &BASIS_EVEN.P);
    copy_point(&Q, &BASIS_EVEN.Q);
    copy_point(&PmQ, &BASIS_EVEN.PmQ);

    printf("Testing order of points\n");
    t = tic();
    dbl_2e(&tmp, e, &P, &A24);
    TOC_clock(t, "Doublings");
    assert(ec_is_zero(&tmp));
    dbl_2e(&tmp, e, &Q, &A24);
    assert(ec_is_zero(&tmp));
    dbl_2e(&tmp, e, &PmQ, &A24);
    assert(ec_is_zero(&tmp));

    printf("Computing Weil pairing\n");
    xADD(&PQ, &P, &Q, &PmQ);
    t = tic();
    weil(&r1, e, &P, &Q, &PQ, &A24);
    TOC_clock(t, "Weil pairing");

    printf("Testing order of Weil pairing\n");
    fp2_set_one(&one);
    fp2_exp_2e(&r2, e - 1, &r1);
    assert(!fp2_is_equal(&r2, &one));
    fp2_exp_2e(&r2, e, &r1);
    assert(fp2_is_equal(&r2, &one));

    printf("Bilinearity tests\n");
    weil(&r2, e, &P, &Q, &PmQ, &A24);
    fp2_inv(&r2);
    assert(fp2_is_equal(&r1, &r2));

    xDBL_A24(&PP, &P, &A24);
    xDBL_A24(&QQ, &Q, &A24);
    xADD(&PPQ, &PQ, &P, &Q);
    xADD(&PQQ, &PQ, &Q, &P);

    weil(&r2, e, &PP, &Q, &PPQ, &A24);
    weil(&r3, e, &P, &QQ, &PQQ, &A24);
    assert(fp2_is_equal(&r2, &r3));
    fp2_sqr(&rr1, &r1);
    assert(fp2_is_equal(&rr1, &r2));

    xADD(&PPP, &PP, &P, &P);
    xADD(&QQQ, &QQ, &Q, &Q);
    xADD(&PPPQ, &PPQ, &P, &PQ);
    xADD(&PQQQ, &PQQ, &Q, &PQ);
    weil(&r2, e, &PPP, &Q, &PPPQ, &A24);
    weil(&r3, e, &P, &QQQ, &PQQQ, &A24);
    assert(fp2_is_equal(&r2, &r3));
    fp2_mul(&rrr1, &rr1, &r1);
    assert(fp2_is_equal(&rrr1, &r2));

    printf("\n\nBenchmarking doublings\n");
    t = tic();
    t0 = rdtsc();
    for (int i = 0; i < bench; ++i) {
        dbl_2e(&tmp, e, &P, &A24);
    }
    t1 = rdtsc();
    ms = (1000. * (float)(clock() - t) / CLOCKS_PER_SEC);
    printf("Average doubling time [%.2f ms]\n", (float)(ms / bench));
    printf("\x1b[34mAvg doubling: %'" PRIu64 " cycles\x1b[0m\n", (t1 - t0) / bench);

    printf("\n\nBenchmarking (Weil) pairings\n");
    t = tic();
    t0 = rdtsc();
    for (int i = 0; i < bench; ++i) {
        weil(&r1, e, &P, &Q, &PQ, &A24);
    }
    t1 = rdtsc();
    ms = (1000. * (float)(clock() - t) / CLOCKS_PER_SEC);
    printf("Average pairing time [%.2f ms]\n", (float)(ms / bench));
    printf("\x1b[34mAvg pairing: %'" PRIu64 " cycles\x1b[0m\n", (t1 - t0) / bench);

    return 1;
}

int
main()
{
    int res = 1;

    printf("Running biextension unit tests\n");

    res = res & biextension_test(1000);

    if (!res) {
        printf("\nSome tests failed!\n");
    } else {
        printf("\nAll tests passed!\n");
    }
    return (!res);
}
