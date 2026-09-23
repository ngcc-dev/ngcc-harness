// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

#include "protocols_helper.h"
#include "bundleprotocols.h"
#include "bundleprotocol_constants.h"
#include "bundleprotocols_internal.h"
#include "bundle_dac.h"
#include <assert.h>
#include <stdio.h>
#include "time.h"

void bundleswap_vertices(bundlevertex_t* A, bundlevertex_t* B, const digit_t option, int len)
{ // Swap vertices
  // If option = 0 then P <- P and Q <- Q, else if option = 0xFF...FF then P <- Q and Q <- P
    bundleswap_points(&(A->A24), &(B->A24), option, len);
    bundleswap_points(&(A->Ps), &(B->Ps), option, len);
    bundleswap_points(&(A->Pt), &(B->Pt), option, len);
    bundleswap_points(&(A->Qs), &(B->Qs), option, len);
    bundleswap_points(&(A->Qt), &(B->Qt), option, len);
}

void bundlevertex_swapshift(bundlevertex_t *res, const bundlevertex_t *inp, int len)
{
    bundleec_point_shift(&(res->A24), &(inp->A24), len);
    bundleec_point_shift(&(res->Qs), &(inp->Ps), len);
    bundleec_point_shift(&(res->Qt), &(inp->Pt), len);
    bundleec_point_shift(&(res->Ps), &(inp->Qs), len);
    bundleec_point_shift(&(res->Pt), &(inp->Qt), len);
}

void bundleeval_action_stra(bundlevertex_t R[2], const int *s, int len)
{
    int strascalar_i;
    bundleec_point_t B24;

    // Isogenies with kernel in E[p+1]
    strascalar_i = 0;
    bundleec_point_t stack_kernel[2][stack_volume];
    int stack_scalar[stack_volume];
    size_t top = 0;
    bundlecopy_point(&(stack_kernel[0][top]), &(R[0].Ps), len);
    bundlecopy_point(&(stack_kernel[1][top]), &(R[1].Ps), len);
    stack_scalar[top] = 0;
    top++;
    int jj = 0, k = 0;
    for(int i = 0; i < P_LEN; i++)
    {
        for(int t = 1; t <= TORSION_ODD_POWERS[i]; t++)
        {
            time_t current_time;
            bool log = false;
            digit_t b = compare_gt(t, s[i]);
            jj++;
            for (int si = 0; si < top; si++)
            {
                bundleswap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b, len);
            }
            bundleswap_vertices(&(R[0]), &(R[1]), b, len);
            while(stack_scalar[top-1] < degree_s_num - jj)
            {
                if(log) {
                time(&current_time);
                printf("%s: stack_scalar[top-1]=%d, degree_s_num - jj=%d\n", ctime(&current_time), (int)(stack_scalar[top-1]), (int)(degree_s_num - jj));
                }
                stack_scalar[top] = stack_scalar[top-1] + strategy_s[k];
               bundlestra_scalar_func_s[strascalar_i](&(stack_kernel[0][top]), &(stack_kernel[0][top-1]), &(R[0].A24), len);
                if (stack_scalar[top] < degree_s_num - jj) // This conditional check does NOT leak any secret information
                {
                   bundlestra_scalar_func_s[strascalar_i](&(stack_kernel[1][top]), &(stack_kernel[1][top-1]), &(R[1].A24), len);
                }
                strascalar_i++;
                top++;
                k++;
                assert(top <= stack_volume);
            }
            top--;
            assert(top >= 0);
            if(log) {
            time(&current_time);
            printf("%s: kps %d start\n", ctime(&current_time), (int)TORSION_ODD_PRIMES[i]);
            }
            bundlekps(i, stack_kernel[0][top], R[0].A24, len);
            if(log) {
            time(&current_time);
            printf("%s: isog %d start\n", ctime(&current_time), (int)TORSION_ODD_PRIMES[i]);
            }
            bundlexisog(&B24, i, R[0].A24, len);
            
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                if(log) {
                time(&current_time);
                printf("%s: eval %d start\n", ctime(&current_time), (int)TORSION_ODD_PRIMES[i]);
                }
                bundlexeval(&(stack_kernel[0][traverse_stack]), i, stack_kernel[0][traverse_stack], R[0].A24, len);
            }
            if(log) {
            time(&current_time);
            printf("%s: eval %d start\n", ctime(&current_time), (int)TORSION_ODD_PRIMES[i]);
            }
            bundlexeval(&(R[0].Pt), i, R[0].Pt, R[0].A24, len);
            if(log) {
            time(&current_time);
            printf("%s: eval %d start\n", ctime(&current_time), (int)TORSION_ODD_PRIMES[i]);
            }
            bundlexeval(&(R[0].Qs), i, R[0].Qs, R[0].A24, len);
            if(log) {
            time(&current_time);
            printf("%s: eval %d start\n", ctime(&current_time), (int)TORSION_ODD_PRIMES[i]);
            }
            bundlexeval(&(R[0].Qt), i, R[0].Qt, R[0].A24, len);
            bundlecopy_point(&(R[0].A24), &B24, len);
            bundlekps_clear(i);
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                // bundlexMULv2(&(stack_kernel[1][traverse_stack]), &(stack_kernel[1][traverse_stack]), &(TORSION_ODD_PRIMES[i]), p_plus_minus_bitlength[i], &(R[1].A24), len);
                bundletorsionscalar_func[i](&(stack_kernel[1][traverse_stack]), &(stack_kernel[1][traverse_stack]), &(R[1].A24), len);
            }
            for (int si = 0; si < top; si++)
            {
                bundleswap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b, len);
            }
            bundleswap_vertices(&(R[0]), &(R[1]), b, len);
        }
    }
    assert(top == 0);

    // Isogenies with kernel in E[p-1]
    strascalar_i = 0;
    top = 0;
    bundlecopy_point(&(stack_kernel[0][top]), &(R[0].Pt), len);
    bundlecopy_point(&(stack_kernel[1][top]), &(R[1].Pt), len);
    stack_scalar[top] = 0;
    top++;
    jj = 0; k = 0;
    for(int i = P_LEN; i < P_LEN + M_LEN; i++)
    {
        for(int t = 1; t <= TORSION_ODD_POWERS[i]; t++)
        {
            digit_t b = compare_gt(t, s[i]);
            jj++;
            for (int si = 0; si < top; si++)
            {
                bundleswap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b, len);
            }
            bundleswap_vertices(&(R[0]), &(R[1]), b, len);
            while(stack_scalar[top-1] < degree_t_num - jj)
            {
                stack_scalar[top] = stack_scalar[top-1] + strategy_t[k];
                // bundlexMULv2(&(stack_kernel[0][top]), &(stack_kernel[0][top-1]), &(strascalar_t[strascalar_i]), strascalar_t_bits[k], &(R[0].A24), len);
                // if (stack_scalar[top] < degree_t_num - jj) // This conditional check does NOT leak any secret information
                // {
                //     bundlexMULv2(&(stack_kernel[1][top]), &(stack_kernel[1][top-1]), &(strascalar_t[strascalar_i]), strascalar_t_bits[k], &(R[1].A24), len);
                // }
                // strascalar_i += strascalar_t_limbs[k];
                bundlestra_scalar_func_t[strascalar_i](&(stack_kernel[0][top]), &(stack_kernel[0][top-1]), &(R[0].A24), len);
                if (stack_scalar[top] < degree_t_num - jj) // This conditional check does NOT leak any secret information
                {
                    bundlestra_scalar_func_t[strascalar_i](&(stack_kernel[1][top]), &(stack_kernel[1][top-1]), &(R[1].A24), len);
                }
                strascalar_i++;
                top++;
                k++;
                assert(top <= stack_volume);
            }
            top--;
            assert(top >= 0);
            bundlekps(i, stack_kernel[0][top], R[0].A24, len);
            bundlexisog(&B24, i, R[0].A24, len);
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                bundlexeval(&(stack_kernel[0][traverse_stack]), i, stack_kernel[0][traverse_stack], R[0].A24, len);
            }
            bundlexeval(&(R[0].Qs), i, R[0].Qs, R[0].A24, len);
            bundlexeval(&(R[0].Qt), i, R[0].Qt, R[0].A24, len);
            bundlecopy_point(&(R[0].A24), &B24, len);
            bundlekps_clear(i);
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                // bundlexMULv2(&(stack_kernel[1][traverse_stack]), &(stack_kernel[1][traverse_stack]), &(TORSION_ODD_PRIMES[i]), p_plus_minus_bitlength[i], &(R[1].A24), len);
                bundletorsionscalar_func[i](&(stack_kernel[1][traverse_stack]), &(stack_kernel[1][traverse_stack]), &(R[1].A24), len);
            }
            for (int si = 0; si < top; si++)
            {
                bundleswap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b, len);
            }
            bundleswap_vertices(&(R[0]), &(R[1]), b, len);
        }
    }
    assert(top == 0);

    #ifndef NDEBUG
    ec_curve_t E0, E1;
    ec_point_t A24_0[len], A24_1[len];
    fp2_t j0, j1;
    bundleec_point_transposition(A24_0, &(R[0].A24), len);
    bundleec_point_transposition(A24_1, &(R[1].A24), len);
    for(int i = 0; i < len; i++)
    {
        A24_to_AC(&E0, &(A24_0[i]));
        A24_to_AC(&E1, &(A24_1[i]));
        ec_j_inv(&j0, &E0);
        ec_j_inv(&j1, &E1);
        assert(fp2_is_equal(&j0, &j1));
        if (!fp2_is_equal(&j0, &j1))
        {
            printf("j_inv not equal\n");
            fflush(stdout);
            exit(1);
        }
    }
    #endif

    if(len == CYCLE_LENGTH)
    {
        ec_isom_t correction;
        ec_curve_t head, tail;
        ec_point_t headA24, tailA24;
        get_point_from_bundle(&headA24, &(R[1].A24), CYCLE_LENGTH - 1);
        get_point_from_bundle(&tailA24, &(R[0].A24), CYCLE_LENGTH - 1);
        A24_to_AC(&head, &headA24);
        A24_to_AC(&tail, &tailA24);
        ec_isomorphism(&correction, &head, &tail);
        ec_point_t headQs, headQt;
        get_point_from_bundle(&headQs, &(R[1].Qs), CYCLE_LENGTH - 1);
        get_point_from_bundle(&headQt, &(R[1].Qt), CYCLE_LENGTH - 1);
        ec_iso_eval(&headQs, &correction);
        ec_iso_eval(&headQt, &correction);
        set_point_to_bundle(&(R[1].Qs), &headQs, CYCLE_LENGTH - 1);
        set_point_to_bundle(&(R[1].Qt), &headQt, CYCLE_LENGTH - 1);
    }

    R[0].Ps = R[1].Qs;
    R[0].Pt = R[1].Qt;
}

void niike_PublicKey(bundlevertex_t* out, secretkey_t s)
{
    secretkey_t copysk;
    memcpy(copysk, s, sizeof(secretkey_t));
    bundlevertex_t R[2];
    R[0] = bundle_base_cycle;
    for(int i = 0; i < ITERATIONS; i++)
    {
        bundlevertex_swapshift(&(R[1]), &(R[0]), CYCLE_LENGTH);
        bundleeval_action_stra(R, copysk, CYCLE_LENGTH);
        for(int j = 0; j < P_LEN + M_LEN; j++)
        {
            copysk[j] -= TORSION_ODD_POWERS[j];
        }
    }
    *out = R[0];
}

void niike_SecretAgreement(fp2_t* out, const bundlevertex_t* in, secretkey_t s)
{
    secretkey_t copysk;
    memcpy(copysk, s, sizeof(secretkey_t));
    bundlevertex_t R[2];
    R[0] = *in;
    for(int i = 0; i < ITERATIONS; i++)
    {
        int tasklength = ITERATIONS - i >= CYCLE_LENGTH ? CYCLE_LENGTH : ITERATIONS - i;
        bundlevertex_swapshift(&(R[1]), &(R[0]), tasklength);
        bundleeval_action_stra(R, copysk, tasklength);
        for(int j = 0; j < P_LEN + M_LEN; j++)
        {
            copysk[j] -= TORSION_ODD_POWERS[j];
        }
    }
    ec_point_t resA24;
    get_point_from_bundle(&resA24, &(R[0].A24), 0);
    ec_curve_t result;
    A24_to_AC(&result, &resA24);
    ec_j_inv(out, &result);
}
