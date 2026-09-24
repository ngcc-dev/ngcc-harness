#include <sqisigndim2.h>
#include <curve_extras.h>


#include <inttypes.h>


// static void fp2_print(char *name, fp2_t const a){
//     fp2_t b;
//     fp2_set(&b, 1);
//     fp2_mul(&b, &b, &a);
//     printf("%s0x", name);
//     for(int i = NWORDS_FIELD - 1; i >=0; i--)
//         printf("%016llx", b.re[i]);
//     printf(" + i*0x");
//     for(int i = NWORDS_FIELD - 1; i >=0; i--)
//         printf("%016llx", b.im[i]);
//     printf("\n");
// }
// static void point_print(char *name, ec_point_t P){
//     fp2_t a;
//     if(fp2_is_zero(&P.z)){
//         printf("%s = INF\n", name);
//     }
//     else{
//     fp2_copy(&a, &P.z);
//     fp2_inv(&a);
//     fp2_mul(&a, &a, &P.x);
//     fp2_print(name, a);
//     }
// }

// static int test_point_order_twof(const ec_point_t *P, const ec_curve_t *E) {
//     ec_point_t test;
//     copy_point(&test, P);
//     if (fp2_is_zero(&test.z)) return 0;
//     for (int i = 0;i<TORSION_PLUS_EVEN_POWER-1;i++) {
//         ec_dbl(&test,E,&test);
//     }
//     if (fp2_is_zero(&test.z)) return 0;
//     ec_dbl(&test,E,&test);
//     return (fp2_is_zero(&test.z));
// }

// static int test_point_order_threef(const ec_point_t *P, const ec_curve_t *E) {
//     ec_point_t test;
//     copy_point(&test, P);
//     digit_t three[NWORDS_ORDER] = {0};
//     three[0] = 3;
//     if (fp2_is_zero(&test.z)) return 0;
//     for (int i = 0;i<TORSION_PLUS_ODD_POWERS[0]-1;i++) {
//         ec_mul(&test, E, three, &test);
//     }
//     if (fp2_is_zero(&test.z)) return 0;
//     ec_mul(&test, E, three, &test);
//     return (fp2_is_zero(&test.z));
// }


static void secret_key_compact_init(secret_key_compact_t *compact) {
    for (int i = 0; i < 6; i++) {
        ibz_init(&(compact->two_part[i]));
        ibz_init(&(compact->three_part[i]));
    }
}

static void secret_key_compact_finalize(secret_key_compact_t *compact) {
    for (int i = 0; i < 6; i++) {
        ibz_finalize(&(compact->two_part[i]));
        ibz_finalize(&(compact->three_part[i]));
    }
}

static void secret_key_compact_zero(secret_key_compact_t *compact) {
    for (int i = 0; i < 5; i++) {
        fp2_set_zero(&(compact->fp2_part[i]));
    }
    for (int i = 0; i < 6; i++) {
        ibz_set(&(compact->two_part[i]), 0);
        ibz_set(&(compact->three_part[i]), 0);
    }
}

static void secret_key_set_compact_from_state(secret_key_t *sk) {
    ec_curve_t normalized;

    ec_curve_init(&normalized);
    copy_curve(&normalized, &(sk->curve));
    ec_normalize_curve(&normalized);
    fp2_copy(&(sk->compact.fp2_part[0]), &(normalized.A));
    fp2_set_zero(&(sk->compact.fp2_part[1]));
    fp2_set_zero(&(sk->compact.fp2_part[2]));
    fp2_set_zero(&(sk->compact.fp2_part[3]));
    fp2_set_zero(&(sk->compact.fp2_part[4]));

    ibz_copy(&(sk->compact.two_part[0]), &(sk->mat_BAcan_to_BA0_two[0][0]));
    ibz_copy(&(sk->compact.two_part[1]), &(sk->mat_BAcan_to_BA0_two[0][1]));
    ibz_copy(&(sk->compact.two_part[2]), &(sk->mat_BAcan_to_BA0_two[1][0]));
    ibz_copy(&(sk->compact.two_part[3]), &(sk->mat_BAcan_to_BA0_two[1][1]));
    ibz_set(&(sk->compact.two_part[4]), 0);
    ibz_set(&(sk->compact.two_part[5]), 0);

    ibz_copy(&(sk->compact.three_part[0]), &(sk->mat_BAcan_to_BA0_three[0][0]));
    ibz_copy(&(sk->compact.three_part[1]), &(sk->mat_BAcan_to_BA0_three[0][1]));
    ibz_copy(&(sk->compact.three_part[2]), &(sk->mat_BAcan_to_BA0_three[1][0]));
    ibz_copy(&(sk->compact.three_part[3]), &(sk->mat_BAcan_to_BA0_three[1][1]));
    ibz_set(&(sk->compact.three_part[4]), 0);
    ibz_set(&(sk->compact.three_part[5]), 0);
}

void public_key_init(public_key_t *pk) {
    ec_curve_init(&(pk->curve));
}

void public_key_finalize(public_key_t *pk) {
    (void)pk;
}

void secret_key_init(secret_key_t *sk) {
    ec_curve_init(&(sk->curve));
    quat_left_ideal_init(&(sk->secret_ideal_two));
    quat_alg_elem_init(&(sk->two_to_three_transporter));
    ibz_mat_2x2_init(&(sk->mat_BAcan_to_BA0_two));
    ibz_mat_2x2_init(&(sk->mat_BAcan_to_BA0_three));
    secret_key_compact_init(&(sk->compact));
    secret_key_compact_zero(&(sk->compact));
}

void secret_key_finalize(secret_key_t *sk) {
    quat_left_ideal_finalize(&(sk->secret_ideal_two));
    quat_alg_elem_finalize(&(sk->two_to_three_transporter));
    ibz_mat_2x2_finalize(&(sk->mat_BAcan_to_BA0_two));
    ibz_mat_2x2_finalize(&(sk->mat_BAcan_to_BA0_three));
    secret_key_compact_finalize(&(sk->compact));
}


void protocols_keygen(public_key_t *pk, secret_key_t *sk) {
    quat_left_ideal_t lideal_odd;
    quat_alg_elem_t gamma;
    ec_isog_even_t two_isogeny_first_half, two_isogeny_second_half;
    ec_isog_odd_t phi_first_half, phi_second_half;
    ec_point_t list_points[3];
    ec_basis_t B_can_two, B_can_three, B_0_two, B_0_three;

    quat_left_ideal_init(&lideal_odd); 
    quat_alg_elem_init(&gamma);

    doublepath(&gamma, &(sk->secret_ideal_two), &lideal_odd, 
    &B_0_three, 
    &B_0_two, &(sk->curve),
    0);

    assert(test_point_order_twof(&(B_0_two.P), &(sk->curve),TORSION_PLUS_EVEN_POWER));

    assert(test_point_order_twof(&(B_0_two.Q), &(sk->curve),TORSION_PLUS_EVEN_POWER));
    assert(test_point_order_twof(&(B_0_two.PmQ), &(sk->curve),TORSION_PLUS_EVEN_POWER));

    assert(test_point_order_threef(&(B_0_three.P), &(sk->curve),TORSION_PLUS_ODD_POWERS[0]));
    assert(test_point_order_threef(&(B_0_three.Q), &(sk->curve),TORSION_PLUS_ODD_POWERS[0]));
    assert(test_point_order_threef(&(B_0_three.PmQ), &(sk->curve),TORSION_PLUS_ODD_POWERS[0]));


    // sk->two_to_three_transporter = conj(gamma)/power_of_2
    quat_alg_conj(&(sk->two_to_three_transporter), &gamma);
    ibz_mul(&(sk->two_to_three_transporter.denom), &(sk->two_to_three_transporter.denom) , &(sk->secret_ideal_two.norm));
    #ifndef NDEBUG
    {
        quat_left_ideal_t lideal_odd_again;
        quat_left_ideal_init(&lideal_odd_again); 

        quat_lideal_mul(&lideal_odd_again, &(sk->secret_ideal_two), &(sk->two_to_three_transporter), &QUATALG_PINFTY, 0); 

        assert(quat_lideal_equals(&lideal_odd_again, &lideal_odd, &QUATALG_PINFTY));

        quat_left_ideal_finalize(&lideal_odd_again); 
    }
    #endif

    //quat_left_ideal_print(&lideal_odd);



    copy_curve(&(pk->curve), &(sk->curve));


    ec_curve_to_basis_2(&B_can_two, &(pk->curve), TORSION_PLUS_EVEN_POWER); // canonical 
    assert(test_point_order_twof(&(B_can_two.P), &(sk->curve),TORSION_PLUS_EVEN_POWER));
    assert(test_point_order_twof(&(B_can_two.Q), &(sk->curve),TORSION_PLUS_EVEN_POWER));
    assert(test_point_order_twof(&(B_can_two.PmQ), &(sk->curve),TORSION_PLUS_EVEN_POWER));
    
    // point_print("pk->curve->basis_two->P = ", B_can_two.P);
    // point_print("pk->curve->basis_two->Q = ", B_can_two.Q);
    // point_print("pk->curve->basis_two->PmQ = ", B_can_two.PmQ);
    change_of_basis_matrix_two(&(sk->mat_BAcan_to_BA0_two), &B_can_two, &B_0_two, &(sk->curve), TORSION_PLUS_EVEN_POWER);
    

    ec_curve_to_basis_3(&B_can_three, &(pk->curve)); // canonical 
    assert(test_point_order_threef(&(B_can_three.P), &(sk->curve),TORSION_PLUS_ODD_POWERS[0]));
    assert(test_point_order_threef(&(B_can_three.Q), &(sk->curve),TORSION_PLUS_ODD_POWERS[0]));
    assert(test_point_order_threef(&(B_can_three.PmQ), &(sk->curve),TORSION_PLUS_ODD_POWERS[0]));

    //point_print("pk->curve->basis_three->P = ", B_can_three.P);
    //point_print("pk->curve->basis_three->Q = ", B_can_three.Q);
    //point_print("pk->curve->basis_three->PmQ = ", B_can_three.PmQ);
    change_of_basis_matrix_three(&(sk->mat_BAcan_to_BA0_three), &B_can_three, &B_0_three, &(sk->curve));

    secret_key_set_compact_from_state(sk);

    quat_left_ideal_finalize(&lideal_odd); 
    quat_alg_elem_finalize(&gamma);
    return;
}
