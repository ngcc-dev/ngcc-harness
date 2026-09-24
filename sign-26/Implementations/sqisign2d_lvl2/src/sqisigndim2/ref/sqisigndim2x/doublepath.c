#include <sqisigndim2.h>
#include <curve_extras.h>
#include <tools.h>

#include <quaternion.h>
#include <ec.h>
#include <endomorphism_action.h>
#include <id2iso.h>
#include <inttypes.h>
#include <locale.h> 
#include <bench.h>
/* Interface */

#define EXPONENT_TWO TORSION_PLUS_EVEN_POWER
#define EXPONENT_THREE TORSION_PLUS_ODD_POWERS[0]
#define POWER_OF_TWO TORSION_PLUS_2POWER
#define POWER_OF_THREE TORSION_PLUS_3POWER

static inline void print_deg(ec_degree_odd_t deg) {    
    #define NUMPP (sizeof(TORSION_ODD_PRIMEPOWERS) / sizeof(*TORSION_ODD_PRIMEPOWERS))

    for (int i = 0; i < NUMPP; i++)
        printf("%d,", deg[i]);
    printf("\n");
}



static int test_point_order_odd_plus(const ec_point_t *P, const ec_curve_t *E) {
    ec_point_t test = *P;
    digit_t scalar[NWORDS_ORDER] = {0};
    ibz_to_digits(scalar, &TORSION_ODD_PLUS);
    ec_mul(&test, E, scalar, &test);
    return (fp2_is_zero(&test.z));
}

static int test_point_order_odd_minus(const ec_point_t *P, const ec_curve_t *E) {
    ec_point_t test = *P;
    digit_t scalar[NWORDS_ORDER] = {0};
    ibz_to_digits(scalar, &TORSION_ODD_MINUS);
    ec_mul(&test, E, scalar, &test);
    return (fp2_is_zero(&test.z));
}


void quat_to_isog_power_of_two(ec_isog_even_t *isog, ibz_vec_2_t *ker_dlog, const quat_alg_elem_t *gamma) {
    quat_left_ideal_t ideal;
    quat_left_ideal_init(&ideal);

    quat_lideal_create_from_primitive(&ideal, gamma, &POWER_OF_TWO, &MAXORD_O0, &QUATALG_PINFTY); 

    id2iso_ideal_to_isogeny_even_dlogs(isog, ker_dlog, &ideal);

    quat_left_ideal_finalize(&ideal);
    return;
}

void quat_to_isog_power_of_three(ec_isog_odd_t *isog, ibz_vec_2_t *ker_dlog, const quat_alg_elem_t *gamma) {
    quat_left_ideal_t ideal;
    quat_left_ideal_init(&ideal);

    quat_lideal_create_from_primitive(&ideal, gamma, &POWER_OF_THREE, &MAXORD_O0, &QUATALG_PINFTY); 

    assert(ibz_cmp(&(ideal.norm), &POWER_OF_THREE) == 0);

    id2iso_ideal_to_isogeny_odd_plus(isog, ker_dlog, &CURVE_E0, &BASIS_ODD_PLUS, &ideal);

    assert(test_point_order_odd_minus(&(isog->ker_minus), &CURVE_E0));
    assert(fp2_is_zero(&((isog->ker_minus).z)));
    assert(!fp2_is_zero(&((isog->ker_plus).z)));
    assert(test_point_order_odd_plus(&(isog->ker_plus), &CURVE_E0));
    quat_left_ideal_finalize(&ideal);
    return;
}


void quat_to_isog_power_of_odd_minus(ec_isog_odd_t *isog, ibz_vec_2_t *ker_dlog, const quat_alg_elem_t *gamma) {
    quat_left_ideal_t ideal;
    quat_left_ideal_init(&ideal);

    quat_lideal_create_from_primitive(&ideal, gamma, &TORSION_ODD_MINUS, &MAXORD_O0, &QUATALG_PINFTY); 

    assert(ibz_cmp(&(ideal.norm), &TORSION_ODD_MINUS) == 0);

    id2iso_ideal_to_isogeny_odd_minus(isog, ker_dlog, &CURVE_E0, &BASIS_ODD_MINUS, &ideal);

    //assert(test_point_order_odd_minus(&(isog->ker_minus), &CURVE_E0));
    assert(!fp2_is_zero(&((isog->ker_minus).z)));
    assert(fp2_is_zero(&((isog->ker_plus).z)));
    assert(test_point_order_odd_minus(&(isog->ker_minus), &CURVE_E0));

    quat_left_ideal_finalize(&ideal);
    return;
}

void isog_init_two(ec_isog_even_t *isog, const ec_curve_t *curve, const ec_point_t *ker, int length) {
    copy_curve(&(isog->curve), curve);
    copy_point(&(isog->kernel), ker);
    //assert(test_point_order_twof(ker, curve));
    isog->length = length;
    return;
}

void isog_init_three(ec_isog_odd_t *isog, const ec_curve_t *curve, const ec_point_t *ker, int length) {
    copy_curve(&(isog->curve), curve);
    copy_point(&(isog->ker_plus), ker);
    //assert(test_point_order_threef(ker, curve));
    ec_set_zero(&(isog->ker_minus));

    #define NUMPP (sizeof(TORSION_ODD_PRIMEPOWERS) / sizeof(*TORSION_ODD_PRIMEPOWERS))

    for (size_t i = 0; i < NUMPP; ++i) {
        (isog->degree)[i] = 0;        
        if (TORSION_ODD_PRIMES[i] == 3){
            assert(length <= TORSION_ODD_POWERS[i]);
            (isog->degree)[i] = length;
        }
    }

    return;
}

// Finds a point P that is independant from vec[0]*basis.P + vec[1]*basis.Q
void complete_three_basis(ec_point_t *P, const ibz_vec_2_t *vec, const ec_basis_t *basis) {
    if (ibz_mod_ui(&(*vec[0]), 3) == 0)
        copy_point(P, &(basis->P));
    else
        copy_point(P, &(basis->Q));
    return;
}

// Finds a point P that is independant from vec[0]*basis.P + vec[1]*basis.Q
void complete_two_basis(ec_point_t *P, const ibz_vec_2_t *vec, const ec_basis_t *basis) {
    if (ibz_mod_ui(&(*vec[0]), 2) == 0)
        copy_point(P, &(basis->P));
    else
        copy_point(P, &(basis->Q));
    return;
}

// TODO(failure case): if intermediate curves (F1,F2,...) have automorphisms, result may be incorrect. Should not happen, but needs sanity check
void doublepath(quat_alg_elem_t *gamma, quat_left_ideal_t *lideal_even, quat_left_ideal_t *lideal_odd, 
    ec_basis_t *basis_three_image, 
    ec_basis_t *basis_two_image, 
    ec_curve_t *E_target,
    int verbose) {

    ibz_t n_gamma, pow_two_square, pow_three_square;
    quat_alg_elem_t gamma_conj;
    ec_isog_even_t dual_two, dual_two_pushed, dual_two_pushed_dual;
    ec_isog_odd_t primal_three, primal_three_pushed, primal_three_pushed_dual;
    ec_point_t list_points[3], ker1, ker2;
    ec_curve_t F1, E1, F2, E2, F2_alt, F1_alt, E_final;
    ec_basis_t basis_two, basis_three;
    ec_isom_t norm_isom;
    ec_isog_even_t primal_two, primal_two_second_half;
    ec_isog_odd_t dual_three, dual_three_second_half;

    ibz_vec_2_t dual_two_ker_dlog, primal_two_ker_dlog, primal_three_ker_dlog, dual_three_ker_dlog;
    ibz_vec_2_init(&dual_two_ker_dlog);
    ibz_vec_2_init(&primal_two_ker_dlog);
    ibz_vec_2_init(&primal_three_ker_dlog);
    ibz_vec_2_init(&dual_three_ker_dlog);

    ibz_init(&n_gamma);
    ibz_init(&pow_two_square);
    ibz_init(&pow_three_square);
    quat_alg_elem_init(&gamma_conj);

    // FIND AN ENDOMORPHISM OF NORM n_gamma = (POWER_OF_TWO*POWER_OF_THREE)^2
    ibz_mul(&pow_two_square, &POWER_OF_TWO, &POWER_OF_TWO);
    ibz_mul(&pow_three_square, &POWER_OF_THREE, &POWER_OF_THREE);
    ibz_mul(&n_gamma, &pow_two_square, &pow_three_square);
    if(verbose) TAC("represent_integer in");
    int found = represent_integer(gamma, &n_gamma, &QUATALG_PINFTY);
    if(verbose) TAC("represent_integer out");
    // TODO(failure case): check that the solution is not divisible by an integer?
    assert(quat_alg_is_primitive(gamma, &MAXORD_O0, &QUATALG_PINFTY));

    #ifndef NDEBUG 
        ibz_t norm_debug;
        ibq_t norm_debug_q;
        ibz_init(&norm_debug);
        ibq_init(&norm_debug_q);
        quat_alg_norm(&norm_debug_q, gamma, &QUATALG_PINFTY);
        ibq_to_ibz(&norm_debug, &norm_debug_q);
        assert(ibz_cmp(&norm_debug, &n_gamma) == 0);
        ibz_finalize(&norm_debug);
        ibq_finalize(&norm_debug_q);
    #endif 

    // COMPUTE THE FIRST HALF OF GAMMA AND OF ITS DUAL
    quat_alg_conj(&gamma_conj, gamma);

    quat_lideal_create_from_primitive(lideal_even, gamma, &pow_two_square, &MAXORD_O0, &QUATALG_PINFTY); 
    quat_lideal_create_from_primitive(lideal_odd, &gamma_conj, &pow_three_square, &MAXORD_O0, &QUATALG_PINFTY); 

    quat_to_isog_power_of_two(&primal_two,&dual_two_ker_dlog, gamma);
    quat_to_isog_power_of_three(&primal_three,&primal_three_ker_dlog, gamma);
    quat_to_isog_power_of_two(&dual_two,&dual_two_ker_dlog, &gamma_conj);
    quat_to_isog_power_of_three(&dual_three,&dual_three_ker_dlog, &gamma_conj);


//     typedef struct ec_isog_odd_t {
//     ec_curve_t curve;
//     ec_point_t ker_plus;    ///< A generator of E[p+1] ∩ ker(φ)
//     ec_point_t ker_minus;   ///< A generator of E[p-1] ∩ ker(φ)
//     ec_degree_odd_t degree; ///< The degree of the isogeny
// } ec_isog_odd_t;

    // TODO: if basis_three_image = NULL, then we dont need the image of BASIS_THREE, and it is (slightly?) faster to compute the image of the following two points instead
    // copy_point(list_points + 0, &(primal_three.ker_plus));
    // complete_three_basis_E0(list_points + 1, &primal_three_ker_dlog);

    copy_point(list_points + 0, &BASIS_THREE.P); 
    copy_point(list_points + 1, &BASIS_THREE.Q); 
    copy_point(list_points + 2, &BASIS_THREE.PmQ);


    ec_eval_even(&F1, &primal_two, list_points, 3);
    // point_print("xP0:=", BASIS_THREE.P);
    // point_print("xQ0:=", BASIS_THREE.Q);
    // point_print("xPmQ0:=", BASIS_THREE.PmQ);

    // point_print("xP_com:=", list_points[0]);
    // point_print("xQ_com:=", list_points[1]);
    // point_print("xPmQ_com:=", list_points[2]);
    // curve_print("acom:=",F1);

    ec_point_t k0;
    copy_point(&k0, &(primal_three).ker_plus);




    copy_point(&(basis_three.P), list_points + 0);
    copy_point(&(basis_three.Q), list_points + 1); 

    copy_point(&(basis_three.PmQ), list_points + 2);
    ec_biscalar_mul_ibz(list_points + 0, &F1, &(primal_three_ker_dlog[0]), &(primal_three_ker_dlog[1]), &basis_three, EXPONENT_THREE);

    isog_init_three(&primal_three_pushed, &F1, list_points + 0, EXPONENT_THREE);
    ec_point_t ker_primal_three_pushed_dual;
    complete_three_basis(&ker_primal_three_pushed_dual, &primal_three_ker_dlog, &basis_three);

    if(verbose) TAC("ec_eval_three in");
    //point_print("ker_primal_three_pushed_dual is", ker_primal_three_pushed_dual);
    ec_eval_three(&E1, &primal_three_pushed, &ker_primal_three_pushed_dual, 1); // point generates the dual of primal_three_pushed
    if(verbose) TAC("ec_eval_three out");
    //point_print("ker_primal_three_pushed_dual is", ker_primal_three_pushed_dual);
    // curve_print("a1:=", E1);


    copy_point(list_points + 0, &BASIS_EVEN.P); 
    copy_point(list_points + 1, &BASIS_EVEN.Q); 
    copy_point(list_points + 2, &BASIS_EVEN.PmQ);
    //assert(test_point_order_twof(&BASIS_EVEN.P, &CURVE_E0));
    //assert(test_point_order_twof(&BASIS_EVEN.Q, &CURVE_E0));
    

    //point_print("BASIS_EVEN.P = ", BASIS_EVEN.P);
    // point_print("BASIS_EVEN.Q = ", BASIS_EVEN.Q);

    if(verbose) TAC("ec_eval_three in");
    ec_eval_three(&F2, &dual_three, list_points, 3);
    if(verbose) TAC("ec_eval_three out");

    copy_point(&(basis_two.P), list_points + 0);
    copy_point(&(basis_two.Q), list_points + 1);
    copy_point(&(basis_two.PmQ), list_points + 2);



    // point_print("xP0:=", BASIS_EVEN.P);
    // point_print("xQ0:=", BASIS_EVEN.Q);
    // point_print("xPmQ0:=", BASIS_EVEN.PmQ);

    // point_print("xP_com:=", list_points[0]);
    // point_print("xQ_com:=", list_points[1]);
    // point_print("xPmQ_com:=", list_points[2]);
    // curve_print("acom:=",F2);

    ec_basis_t basis_two_mid;

    copy_point(&(basis_two_mid.P), list_points + 0);
    copy_point(&(basis_two_mid.Q), list_points + 1);
    copy_point(&(basis_two_mid.PmQ), list_points + 2);



    ec_biscalar_mul_ibz(list_points + 0, &F2,
    &(dual_two_ker_dlog[0]), &(dual_two_ker_dlog[1]), &basis_two, EXPONENT_TWO);

    isog_init_two(&dual_two_pushed, &F2, list_points + 0, EXPONENT_TWO);

    ec_point_t ker_dual_two_pushed_dual;

    complete_two_basis(&ker_dual_two_pushed_dual, &dual_two_ker_dlog, &basis_two);

    ec_eval_even(&E2, &dual_two_pushed, &ker_dual_two_pushed_dual, 1);
    
    //ec_eval_small_chain(&E2, list_points + 0, EXPONENT_TWO,  &ker_dual_two_pushed_dual, 1);




    // FIRST HALF OF GAMMA AND FIRST HALF OF ITS DUAL HAVE ISOMORPHIC TARGETS    
    #ifndef NDEBUG 
        fp2_t j_R,j_L;
        ec_j_inv(&j_R, &E1);
        ec_j_inv(&j_L, &E2);
        assert(fp2_is_equal(&j_R,&j_L));
    #endif 
    ec_isom_t isom_E1_E2;
    ec_isomorphism(&isom_E1_E2, &E1, &E2);


    // PUSH THINGS AROUND TO GET THE 3-PART OF GAMMA_DUAL
    if(basis_two_image){    
        ec_point_t ker_primal_three_pushed_dual_E2;
        copy_point(&ker_primal_three_pushed_dual_E2, &ker_primal_three_pushed_dual);
        ec_iso_eval(&ker_primal_three_pushed_dual_E2, &isom_E1_E2);

        isog_init_two(&dual_two_pushed_dual, &E2, &ker_dual_two_pushed_dual, EXPONENT_TWO);


        ec_point_t ker_dual_three_second_half;
        copy_point(&ker_dual_three_second_half, &ker_primal_three_pushed_dual_E2);
        ec_eval_even(&F2_alt, &dual_two_pushed_dual, &ker_dual_three_second_half, 1);

        #ifndef NDEBUG 
            ec_j_inv(&j_R, &F2);
            ec_j_inv(&j_L, &F2_alt);
            assert(fp2_is_equal(&j_R,&j_L));
        #endif 
        ec_isom_t isom_F2_alt_F2;
        ec_isomorphism(&isom_F2_alt_F2, &F2_alt, &F2);
        ec_iso_eval(&ker_dual_three_second_half, &isom_F2_alt_F2);

        isog_init_three(&dual_three_second_half, &F2, &ker_dual_three_second_half, EXPONENT_THREE);
        ec_point_t km;
        copy_point(&km, &(ker_dual_three_second_half));


        copy_point(list_points + 0, &basis_two.P); 
        copy_point(list_points + 1, &basis_two.Q); 
        copy_point(list_points + 2, &basis_two.PmQ);

        if(verbose) TAC("ec_eval_three in");
        ec_eval_three(&E_final, &dual_three_second_half, list_points, 3);
        if(verbose) TAC("ec_eval_three out");

        ec_curve_normalize(&E_final, &norm_isom, &E_final);
        ec_iso_eval(list_points + 0, &norm_isom);
        ec_iso_eval(list_points + 1, &norm_isom);
        ec_iso_eval(list_points + 2, &norm_isom);

        copy_point(&(basis_two_image->P), list_points + 0);
        copy_point(&(basis_two_image->Q), list_points + 1);
        copy_point(&(basis_two_image->PmQ), list_points + 2);


        if(E_target)
            // TODO: normalize E_target with ec_curve_normalize
            copy_curve(E_target, &E_final);
    }


    if(basis_three_image){  
        // PUSH THINGS AROUND TO GET THE 2-PART OF GAMMA
        ec_point_t ker_dual_two_pushed_dual_E1;
        copy_point(&ker_dual_two_pushed_dual_E1, &ker_dual_two_pushed_dual);
        ec_iso_inv(&isom_E1_E2);
        ec_iso_eval(&ker_dual_two_pushed_dual_E1, &isom_E1_E2);

        isog_init_three(&primal_three_pushed_dual, &E1, &ker_primal_three_pushed_dual, EXPONENT_THREE);

        ec_point_t ker_primal_two_second_half;
        copy_point(&ker_primal_two_second_half, &ker_dual_two_pushed_dual_E1);
        
        if(verbose) TAC("ec_eval_three in");
        ec_eval_three(&F1_alt, &primal_three_pushed_dual, &ker_primal_two_second_half, 1);
        if(verbose) TAC("ec_eval_three out");

        #ifndef NDEBUG
            ec_curve_t F1_alt2;

            ec_eval_odd(&F1_alt2, &primal_three_pushed_dual, NULL, 0);

            ec_j_inv(&j_R, &F1);
            ec_j_inv(&j_L, &F1_alt);
            //assert(fp2_is_equal(&j_R,&j_L));

            ec_j_inv(&j_L, &F1_alt2);
            //assert(fp2_is_equal(&j_R,&j_L));
        #endif 
        ec_isom_t isom_F1_alt_F1;
        ec_isomorphism(&isom_F1_alt_F1, &F1_alt, &F1);

        ec_iso_eval(&ker_primal_two_second_half, &isom_F1_alt_F1);

        isog_init_two(&primal_two_second_half, &F1, &ker_primal_two_second_half, EXPONENT_TWO);

  
        copy_point(list_points + 0, &basis_three.P); 
        copy_point(list_points + 1, &basis_three.Q); 
        copy_point(list_points + 2, &basis_three.PmQ);

        ec_eval_even(&E_final, &primal_two_second_half, list_points, 3);

        ec_curve_normalize(&E_final, &norm_isom, &E_final);
        ec_iso_eval(list_points + 0, &norm_isom);
        ec_iso_eval(list_points + 1, &norm_isom);
        ec_iso_eval(list_points + 2, &norm_isom);

        copy_point(&(basis_three_image->P), list_points + 0);
        copy_point(&(basis_three_image->Q), list_points + 1);
        copy_point(&(basis_three_image->PmQ), list_points + 2);


        #ifndef NDEBUG 
        if(E_target && basis_two_image) {
            assert(fp2_is_equal(&E_final.A, &(E_target->A)));
            assert(fp2_is_equal(&E_final.C, &(E_target->C)));
        }
        #endif

        if(E_target && !basis_two_image)
            copy_curve(E_target, &E_final);
    }


    quat_alg_elem_finalize(&gamma_conj);
    ibz_finalize(&n_gamma);
    ibz_finalize(&pow_two_square);
    ibz_finalize(&pow_three_square);
    ibz_vec_2_finalize(&dual_two_ker_dlog);
    ibz_vec_2_finalize(&primal_two_ker_dlog);
    ibz_vec_2_finalize(&primal_three_ker_dlog);
    ibz_vec_2_finalize(&dual_three_ker_dlog);
    return;
}



void doublepath_com(quat_alg_elem_t *gamma, quat_left_ideal_t *lideal_even, quat_left_ideal_t *lideal_odd, 
    ec_basis_t *basis_three_image, 
    ec_basis_t *basis_two_mid, 
    ec_basis_t *basis_two_image, 
    ec_curve_t *E_mid,
    ec_point_t * k0,
    ec_point_t * km,
    ec_curve_t *E_target,
    ibz_t *a3,
    ibz_t *b3,
    int verbose) {
    //!is_fast && !is_both && return a24_3, a24, K3, xK, xP2e3, xQ2e3, xPQ2e3, xP2e3d, xQ2e3d, xPQ2e3d, J3
    ibz_t n_gamma, pow_two_square, pow_three_square;
    quat_alg_elem_t gamma_conj;
    ec_isog_even_t dual_two, dual_two_pushed, dual_two_pushed_dual;
    ec_isog_odd_t primal_three, primal_three_pushed, primal_three_pushed_dual;
    ec_point_t list_points[3];
    ec_curve_t F1, E1, F2, E2, F2_alt, F1_alt, E_final;
    ec_basis_t basis_two, basis_three;
    ec_isom_t norm_isom;
    ec_isog_even_t primal_two, primal_two_second_half;
    ec_isog_odd_t dual_three, dual_three_second_half;

    ibz_vec_2_t dual_two_ker_dlog, primal_two_ker_dlog, primal_three_ker_dlog, dual_three_ker_dlog;
    ibz_vec_2_init(&dual_two_ker_dlog);
    ibz_vec_2_init(&primal_two_ker_dlog);
    ibz_vec_2_init(&primal_three_ker_dlog);
    ibz_vec_2_init(&dual_three_ker_dlog);

    ibz_init(&n_gamma);
    ibz_init(&pow_two_square);
    ibz_init(&pow_three_square);
    quat_alg_elem_init(&gamma_conj);

    // FIND AN ENDOMORPHISM OF NORM n_gamma = (POWER_OF_TWO*POWER_OF_THREE)^2
    ibz_mul(&pow_two_square, &POWER_OF_TWO, &POWER_OF_TWO);
    ibz_mul(&pow_three_square, &POWER_OF_THREE, &POWER_OF_THREE);
    ibz_mul(&n_gamma, &pow_two_square, &pow_three_square);

    if(verbose) TAC("represent_integer in");
    int found = represent_integer(gamma, &n_gamma, &QUATALG_PINFTY);
    if(verbose) TAC("represent_integer out");
    // TODO(failure case): check that the solution is not divisible by an integer?
    assert(quat_alg_is_primitive(gamma, &MAXORD_O0, &QUATALG_PINFTY));

    #ifndef NDEBUG 
        ibz_t norm_debug;
        ibq_t norm_debug_q;
        ibz_init(&norm_debug);
        ibq_init(&norm_debug_q);
        quat_alg_norm(&norm_debug_q, gamma, &QUATALG_PINFTY);
        ibq_to_ibz(&norm_debug, &norm_debug_q);
        assert(ibz_cmp(&norm_debug, &n_gamma) == 0);
        ibz_finalize(&norm_debug);
        ibq_finalize(&norm_debug_q);
    #endif 

    // COMPUTE THE FIRST HALF OF GAMMA AND OF ITS DUAL
    quat_alg_conj(&gamma_conj, gamma);

    quat_lideal_create_from_primitive(lideal_even, gamma, &pow_two_square, &MAXORD_O0, &QUATALG_PINFTY); 
    quat_lideal_create_from_primitive(lideal_odd, &gamma_conj, &pow_three_square, &MAXORD_O0, &QUATALG_PINFTY); 

    quat_to_isog_power_of_two(&primal_two,&dual_two_ker_dlog, gamma);
    quat_to_isog_power_of_three(&primal_three,&primal_three_ker_dlog, gamma);
    quat_to_isog_power_of_two(&dual_two,&dual_two_ker_dlog, &gamma_conj);
    quat_to_isog_power_of_three(&dual_three,&dual_three_ker_dlog, &gamma_conj);
    copy_point(k0, &(dual_three).ker_plus);
    ibz_copy(a3, &dual_three_ker_dlog[0]);
    ibz_copy(b3, &dual_three_ker_dlog[1]);

    // TODO: if basis_three_image = NULL, then we dont need the image of BASIS_THREE, and it is (slightly?) faster to compute the image of the following two points instead
    // copy_point(list_points + 0, &(primal_three.ker_plus));
    // complete_three_basis_E0(list_points + 1, &primal_three_ker_dlog);

    copy_point(list_points + 0, &BASIS_THREE.P); 
    copy_point(list_points + 1, &BASIS_THREE.Q); 
    copy_point(list_points + 2, &BASIS_THREE.PmQ);


    ec_eval_even(&F1, &primal_two, list_points, 3);

    copy_point(&(basis_three.P), list_points + 0);
    copy_point(&(basis_three.Q), list_points + 1);
    copy_point(&(basis_three.PmQ), list_points + 2);
    ec_biscalar_mul_ibz(list_points + 0, &F1, &(primal_three_ker_dlog[0]), &(primal_three_ker_dlog[1]), &basis_three, EXPONENT_THREE);

    isog_init_three(&primal_three_pushed, &F1, list_points + 0, EXPONENT_THREE);

    ec_point_t ker_primal_three_pushed_dual;
    complete_three_basis(&ker_primal_three_pushed_dual, &primal_three_ker_dlog, &basis_three);

    if(verbose) TAC("ec_eval_three in");
    ec_eval_three(&E1, &primal_three_pushed, &ker_primal_three_pushed_dual, 1); // point generates the dual of primal_three_pushed
    if(verbose) TAC("ec_eval_three out");


    copy_point(list_points + 0, &BASIS_EVEN.P); 
    copy_point(list_points + 1, &BASIS_EVEN.Q); 
    copy_point(list_points + 2, &BASIS_EVEN.PmQ);

    //assert(test_point_order_twof(&BASIS_EVEN.P, &CURVE_E0));
    //assert(test_point_order_twof(&BASIS_EVEN.Q, &CURVE_E0));
    

    //point_print("BASIS_EVEN.P = ", BASIS_EVEN.P);
    // point_print("BASIS_EVEN.Q = ", BASIS_EVEN.Q);

    if(verbose) TAC("ec_eval_three in");
    ec_eval_three(&F2, &dual_three, list_points, 3);
    if(verbose) TAC("ec_eval_three out");

    copy_point(&(basis_two.P), list_points + 0);
    copy_point(&(basis_two.Q), list_points + 1);
    copy_point(&(basis_two.PmQ), list_points + 2);


    copy_point(&(basis_two_mid->P), list_points + 0);
    copy_point(&(basis_two_mid->Q), list_points + 1);
    copy_point(&(basis_two_mid->PmQ), list_points + 2); 

    ec_biscalar_mul_ibz(list_points + 0, &F2,
    &(dual_two_ker_dlog[0]), &(dual_two_ker_dlog[1]), &basis_two, EXPONENT_TWO);

    isog_init_two(&dual_two_pushed, &F2, list_points + 0, EXPONENT_TWO);

    ec_point_t ker_dual_two_pushed_dual;

    complete_two_basis(&ker_dual_two_pushed_dual, &dual_two_ker_dlog, &basis_two);

    //assert(test_point_order_twof(&ker_dual_two_pushed_dual, &F2));
    ec_eval_even(&E2, &dual_two_pushed, &ker_dual_two_pushed_dual, 1);
    //assert(test_point_order_twof(&ker_dual_two_pushed_dual, &E2));

    // FIRST HALF OF GAMMA AND FIRST HALF OF ITS DUAL HAVE ISOMORPHIC TARGETS    
    #ifndef NDEBUG 
        fp2_t j_R,j_L;
        ec_j_inv(&j_R, &E1);
        ec_j_inv(&j_L, &E2);
        assert(fp2_is_equal(&j_R,&j_L));
    #endif 
    ec_isom_t isom_E1_E2;
    ec_isomorphism(&isom_E1_E2, &E1, &E2);


    // PUSH THINGS AROUND TO GET THE 3-PART OF GAMMA_DUAL
    if(basis_two_image){  
        ec_point_t ker_primal_three_pushed_dual_E2;
        copy_point(&ker_primal_three_pushed_dual_E2, &ker_primal_three_pushed_dual);
        ec_iso_eval(&ker_primal_three_pushed_dual_E2, &isom_E1_E2);

        isog_init_two(&dual_two_pushed_dual, &E2, &ker_dual_two_pushed_dual, EXPONENT_TWO);


        ec_point_t ker_dual_three_second_half;
        copy_point(&ker_dual_three_second_half, &ker_primal_three_pushed_dual_E2);
        ec_eval_even(&F2_alt, &dual_two_pushed_dual, &ker_dual_three_second_half, 1);

        #ifndef NDEBUG 
            ec_j_inv(&j_R, &F2);
            ec_j_inv(&j_L, &F2_alt);
            assert(fp2_is_equal(&j_R,&j_L));
        #endif 
        ec_isom_t isom_F2_alt_F2;
        ec_isomorphism(&isom_F2_alt_F2, &F2_alt, &F2);
        ec_iso_eval(&ker_dual_three_second_half, &isom_F2_alt_F2);

        copy_point(km, &(ker_dual_three_second_half));
        copy_curve(E_mid, &F2);
        isog_init_three(&dual_three_second_half, &F2, &ker_dual_three_second_half, EXPONENT_THREE);


        copy_point(list_points + 0, &basis_two.P); 
        copy_point(list_points + 1, &basis_two.Q); 
        copy_point(list_points + 2, &basis_two.PmQ);

        if(verbose) TAC("ec_eval_three in");
        ec_eval_three(&E_final, &dual_three_second_half, list_points, 3);
        if(verbose) TAC("ec_eval_three out");


        copy_point(&(basis_two_image->P), list_points + 0);
        copy_point(&(basis_two_image->Q), list_points + 1);
        copy_point(&(basis_two_image->PmQ), list_points + 2);
        copy_curve(E_target, &E_final);

        // ec_curve_normalize(&E_final, &norm_isom, &E_final);
        // ec_iso_eval(list_points + 0, &norm_isom);
        // ec_iso_eval(list_points + 1, &norm_isom);
        // ec_iso_eval(list_points + 2, &norm_isom);

        // copy_point(&(basis_two_image->P), list_points + 0);
        // copy_point(&(basis_two_image->Q), list_points + 1);
        // copy_point(&(basis_two_image->PmQ), list_points + 2);


        // if(E_target)
        //     // TODO: normalize E_target with ec_curve_normalize
        //     copy_curve(E_target, &E_final);
    }


    if(basis_three_image){  
        // PUSH THINGS AROUND TO GET THE 2-PART OF GAMMA
        ec_point_t ker_dual_two_pushed_dual_E1;
        copy_point(&ker_dual_two_pushed_dual_E1, &ker_dual_two_pushed_dual);
        ec_iso_inv(&isom_E1_E2);
        ec_iso_eval(&ker_dual_two_pushed_dual_E1, &isom_E1_E2);

        isog_init_three(&primal_three_pushed_dual, &E1, &ker_primal_three_pushed_dual, EXPONENT_THREE);

        ec_point_t ker_primal_two_second_half;
        copy_point(&ker_primal_two_second_half, &ker_dual_two_pushed_dual_E1);
        
        if(verbose) TAC("ec_eval_three in");
        ec_eval_three(&F1_alt, &primal_three_pushed_dual, &ker_primal_two_second_half, 1);
        if(verbose) TAC("ec_eval_three out");

        #ifndef NDEBUG
            ec_curve_t F1_alt2;

            ec_eval_odd(&F1_alt2, &primal_three_pushed_dual, NULL, 0);

            ec_j_inv(&j_R, &F1);
            ec_j_inv(&j_L, &F1_alt);
            assert(fp2_is_equal(&j_R,&j_L));

            ec_j_inv(&j_L, &F1_alt2);
            assert(fp2_is_equal(&j_R,&j_L));
        #endif 
        ec_isom_t isom_F1_alt_F1;
        ec_isomorphism(&isom_F1_alt_F1, &F1_alt, &F1);
        ec_iso_eval(&ker_primal_two_second_half, &isom_F1_alt_F1);

        isog_init_two(&primal_two_second_half, &F1, &ker_primal_two_second_half, EXPONENT_TWO);

  
        copy_point(list_points + 0, &basis_three.P); 
        copy_point(list_points + 1, &basis_three.Q); 
        copy_point(list_points + 2, &basis_three.PmQ);

        ec_eval_even(&E_final, &primal_two_second_half, list_points, 3);

        ec_curve_normalize(&E_final, &norm_isom, &E_final);
        ec_iso_eval(list_points + 0, &norm_isom);
        ec_iso_eval(list_points + 1, &norm_isom);
        ec_iso_eval(list_points + 2, &norm_isom);

        copy_point(&(basis_three_image->P), list_points + 0);
        copy_point(&(basis_three_image->Q), list_points + 1);
        copy_point(&(basis_three_image->PmQ), list_points + 2);


        #ifndef NDEBUG 
        if(E_target && basis_two_image) {
            assert(fp2_is_equal(&E_final.A, &(E_target->A)));
            assert(fp2_is_equal(&E_final.C, &(E_target->C)));
        }
        #endif

        if(E_target && !basis_two_image)
            copy_curve(E_target, &E_final);
    }

    quat_alg_elem_finalize(&gamma_conj);
    ibz_finalize(&n_gamma);
    ibz_finalize(&pow_two_square);
    ibz_finalize(&pow_three_square);
    ibz_vec_2_finalize(&dual_two_ker_dlog);
    ibz_vec_2_finalize(&primal_two_ker_dlog);
    ibz_vec_2_finalize(&primal_three_ker_dlog);
    ibz_vec_2_finalize(&dual_three_ker_dlog);
    return;
}