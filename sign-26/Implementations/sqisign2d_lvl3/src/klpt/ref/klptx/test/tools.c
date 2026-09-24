#include "klpt_tests.h"

int
klpt_test_represent_integer()
{
    int found = 0;
    ibz_t p, temp;
    ibz_t M, M_begin;
    quat_alg_t Bpoo;
    quat_alg_elem_t gamma;

    quat_alg_elem_init(&gamma);
    ibz_init(&M);
    ibz_init(&M_begin);
    ibz_init(&p);
    ibz_init(&temp);

    int exp = 128;
    found = generate_random_prime(&p, 1, 2 * exp);
    quat_alg_init_set(&Bpoo, &p);

    found = found && (generate_random_prime(&M, 1, exp));
    ibz_pow(&temp, &ibz_const_two, exp + 15);
    ibz_mul(&M, &M, &temp);
    ibz_copy(&M_begin, &M);

    found = found && represent_integer(&gamma, &M, &Bpoo);
    if (!found) {
        printf("repesent integer did not find anything \n");
    }

    ibq_t norm;
    ibq_init(&norm);
    ibz_t ibz_norm;
    ibz_init(&ibz_norm);
    quat_alg_norm(&norm, &gamma, &Bpoo);

    found = found && ibq_to_ibz(&ibz_norm, &norm);

    found = found && (ibz_cmp(&ibz_norm, &M) == 0);
    if (!found) {
        ibz_printf("unequality of norm \n %Zd \n %Zd \n", ibz_norm, M);
    }

    ibz_t remainder;
    ibz_init(&remainder);
    ibz_div(&temp, &remainder, &M_begin, &M);

    found = found && (ibz_cmp(&remainder, &ibz_const_zero) == 0);

    quat_alg_elem_finalize(&gamma);
    ibz_finalize(&M);
    quat_alg_finalize(&Bpoo);
    ibz_finalize(&p);
    ibz_finalize(&temp);
    ibz_finalize(&M_begin);
    ibz_finalize(&remainder);
    ibq_finalize(&norm);
    ibz_finalize(&ibz_norm);
    return found;
}

int
klpt_test_tools()
{

    int res = 1;
    printf("Running klpt tests for represent integer \n \n");

    for (int i = 0; i < 3; i++) {
        res = res && klpt_test_represent_integer();
    }
    if (!res) {
        printf("KLPT unit test represent_integer failed\n");
    }

    return res;
}
