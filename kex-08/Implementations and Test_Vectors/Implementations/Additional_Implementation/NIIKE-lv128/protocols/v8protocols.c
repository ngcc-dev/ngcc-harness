// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

#include "protocols_helper.h"
#include "protocol_constants.h"
#include "v8protocols.h"
#include "v8protocols_helper.h"
#include "dac.h"
#include "v8dac.h"
#include <stdio.h>
#include "encode_sizes.h"

void make_SecretKey(secretkey_t sk, DRNG_ctx *drng)
{
    for(int i = 0; i < P_LEN + M_LEN; i++)
    {
        do
        {
            get_random_number(drng, (unsigned char *)&(sk[i]), 8*sizeof(sk[i]));
            sk[i] &= secretkey_mask[i];
        } while (sk[i] > ITERATIONS * TORSION_ODD_POWERS[i]);
    }
}

void print_SecretKey(const secretkey_t sk)
{
    for(int i = 0; i < P_LEN + M_LEN - 1; i++)
    {
        printf("%d,", sk[i]);
    }
    printf("%d", sk[P_LEN + M_LEN - 1]);
    printf("\n");
}

void print_PublicKey(const orient_cycle_t *pk)
{
    print_cycle(pk);
    printf("\n");
}

void print_SecretAgreement(const fp2_t a)
{
    fp2_print(a);
    printf("\n");
}

void v8eval_action_stra(v8vertex_t R[2], const int *s)
{
    int strascalar_i;
    v8ec_point_t B24;

    // Isogenies with kernel in E[p+1]
    strascalar_i = 0;
    v8ec_point_t stack_kernel[2][stack_volume];
    int stack_scalar[stack_volume];
    size_t top = 0;
    v8copy_point(&(stack_kernel[0][top]), &(R[0].Ps));
    v8copy_point(&(stack_kernel[1][top]), &(R[1].Ps));
    stack_scalar[top] = 0;
    top++;
    int jj = 0, k = 0;
    for(int i = 0; i < P_LEN; i++)
    {
        for(int t = 1; t <= TORSION_ODD_POWERS[i]; t++)
        {
            __m512i b = v8compare_gt(t, s[i]);
            jj++;
            for (int si = 0; si < top; si++)
            {
                v8swap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b);
            }
            v8swap_vertices(&(R[0]), &(R[1]), b);
            while(stack_scalar[top-1] < degree_s_num - jj)
            {
                stack_scalar[top] = stack_scalar[top-1] + strategy_s[k];
                v8stra_scalar_func_s[strascalar_i](&(stack_kernel[0][top]), &(stack_kernel[0][top-1]), &(R[0].A24));
                if (stack_scalar[top] < degree_s_num - jj) // This conditional check does NOT leak any secret information
                {
                    v8stra_scalar_func_s[strascalar_i](&(stack_kernel[1][top]), &(stack_kernel[1][top-1]), &(R[1].A24));
                }
                strascalar_i++;
                top++;
                k++;
                assert(top <= stack_volume);
            }
            top--;
            assert(top >= 0);
            v8kps(i, &(stack_kernel[0][top]), &(R[0].A24));
            v8xisog(&B24, i, &(R[0].A24));
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                v8xeval(&(stack_kernel[0][traverse_stack]), i, &(stack_kernel[0][traverse_stack]), &(R[0].A24));
            }
            v8xeval(&(R[0].Pt), i, &(R[0].Pt), &(R[0].A24));
            v8xeval(&(R[0].Qs), i, &(R[0].Qs), &(R[0].A24));
            v8xeval(&(R[0].Qt), i, &(R[0].Qt), &(R[0].A24));
            v8copy_point(&(R[0].A24), &B24);
            v8kps_clear(i);
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                v8torsionscalar_func[i](&(stack_kernel[1][traverse_stack]), &(stack_kernel[1][traverse_stack]), &(R[1].A24));
            }
            for (int si = 0; si < top; si++)
            {
                v8swap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b);
            }
            v8swap_vertices(&(R[0]), &(R[1]), b);
        }
    }
    assert(top == 0);

    // Isogenies with kernel in E[p-1]
    strascalar_i = 0;
    top = 0;
    v8copy_point(&(stack_kernel[0][top]), &(R[0].Pt));
    v8copy_point(&(stack_kernel[1][top]), &(R[1].Pt));
    stack_scalar[top] = 0;
    top++;
    jj = 0; k = 0;
    for(int i = P_LEN; i < P_LEN + M_LEN; i++)
    {
        for(int t = 1; t <= TORSION_ODD_POWERS[i]; t++)
        {
            __m512i b = v8compare_gt(t, s[i]);
            jj++;
            for (int si = 0; si < top; si++)
            {
                v8swap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b);
            }
            v8swap_vertices(&(R[0]), &(R[1]), b);
            while(stack_scalar[top-1] < degree_t_num - jj)
            {
                stack_scalar[top] = stack_scalar[top-1] + strategy_t[k];
                v8stra_scalar_func_t[strascalar_i](&(stack_kernel[0][top]), &(stack_kernel[0][top-1]), &(R[0].A24));
                if (stack_scalar[top] < degree_t_num - jj) // This conditional check does NOT leak any secret information
                {
                    v8stra_scalar_func_t[strascalar_i](&(stack_kernel[1][top]), &(stack_kernel[1][top-1]), &(R[1].A24));
                }
                strascalar_i++;
                top++;
                k++;
                assert(top <= stack_volume);
            }
            top--;
            assert(top >= 0);
            v8kps(i, &(stack_kernel[0][top]), &(R[0].A24));
            v8xisog(&B24, i, &(R[0].A24));
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                v8xeval(&(stack_kernel[0][traverse_stack]), i, &(stack_kernel[0][traverse_stack]), &(R[0].A24));
            }
            v8xeval(&(R[0].Qs), i, &(R[0].Qs), &(R[0].A24));
            v8xeval(&(R[0].Qt), i, &(R[0].Qt), &(R[0].A24));
            v8copy_point(&(R[0].A24), &B24);
            v8kps_clear(i);
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                v8torsionscalar_func[i](&(stack_kernel[1][traverse_stack]), &(stack_kernel[1][traverse_stack]), &(R[1].A24));
            }
            for (int si = 0; si < top; si++)
            {
                v8swap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b);
            }
            v8swap_vertices(&(R[0]), &(R[1]), b);
        }
    }
    assert(top == 0);
}

void eval_action_vertex_stra_8(vertex_t *out1, vertex_t *out2, vertex_t *out3, vertex_t *out4, vertex_t *out5, vertex_t *out6, vertex_t *out7, vertex_t *out8,
                               const vertex_t *in1, const vertex_t *in2, const vertex_t *in3, const vertex_t *in4, const vertex_t *in5, const vertex_t *in6, const vertex_t *in7, const vertex_t *in8, const vertex_t *in9,
                               const int *s, bool istail)
{
    v8vertex_t R[2];
    v8vertex_pack8(&(R[0]), in1, in2, in3, in4, in5, in6, in7, in8);
    v8vertex_swappack8(&(R[1]), in2, in3, in4, in5, in6, in7, in8, in9);

    v8eval_action_stra(R, s);

    #ifndef NDEBUG
    v8ec_curve_t E1, E2;
    v8A24_to_AC(&E1, &(R[0].A24));
    v8A24_to_AC(&E2, &(R[1].A24));
    fp2_t j11, j12, j13, j14, j15, j16, j17, j18, j21, j22, j23, j24, j25, j26, j27, j28;
    ec_curve_t E11, E12, E13, E14, E15, E16, E17, E18;
    ec_curve_t E21, E22, E23, E24, E25, E26, E27, E28;
    v8fp2_unpack8(&(E11.A), &(E12.A), &(E13.A), &(E14.A), &(E15.A), &(E16.A), &(E17.A), &(E18.A), &(E1.A));
    v8fp2_unpack8(&(E21.A), &(E22.A), &(E23.A), &(E24.A), &(E25.A), &(E26.A), &(E27.A), &(E28.A), &(E2.A));
    v8fp2_unpack8(&(E11.C), &(E12.C), &(E13.C), &(E14.C), &(E15.C), &(E16.C), &(E17.C), &(E18.C), &(E1.C));
    v8fp2_unpack8(&(E21.C), &(E22.C), &(E23.C), &(E24.C), &(E25.C), &(E26.C), &(E27.C), &(E28.C), &(E2.C));
    ec_j_inv(&j11, &E11); ec_j_inv(&j21, &E21);
    ec_j_inv(&j12, &E12); ec_j_inv(&j22, &E22);
    ec_j_inv(&j13, &E13); ec_j_inv(&j23, &E23);
    ec_j_inv(&j14, &E14); ec_j_inv(&j24, &E24);
    ec_j_inv(&j15, &E15); ec_j_inv(&j25, &E25);
    ec_j_inv(&j16, &E16); ec_j_inv(&j26, &E26);
    ec_j_inv(&j17, &E17); ec_j_inv(&j27, &E27);
    ec_j_inv(&j18, &E18); ec_j_inv(&j28, &E28);
    assert(fp2_is_equal(&j11, &j21));
    assert(fp2_is_equal(&j12, &j22));
    assert(fp2_is_equal(&j13, &j23));
    assert(fp2_is_equal(&j14, &j24));
    assert(fp2_is_equal(&j15, &j25));
    assert(fp2_is_equal(&j16, &j26));
    assert(fp2_is_equal(&j17, &j27));
    assert(fp2_is_equal(&j18, &j28));
    #endif

    v8vertex_output8(out1, out2, out3, out4, out5, out6, out7, out8, &(R[0]), &(R[1]));

    if (istail)
    {
        ec_point_t t1, t2, t3, t4, t5, t6, t7, t8;
        v8ec_point_unpack8(&t1, &t2, &t3, &t4, &t5, &t6, &t7, &t8, &(R[1].A24));
        ec_isom_t correction;
        ec_curve_t head, tail;
        A24_to_AC(&head, &(t8));
        A24_to_AC(&tail, &(out8->A24));
        ec_isomorphism(&correction, &head, &tail);
        ec_iso_eval(&(out8->Ps), &correction);
        ec_iso_eval(&(out8->Pt), &correction);
    }
}

void eval_action_vertex_stra_7(vertex_t *out1, vertex_t *out2, vertex_t *out3, vertex_t *out4, vertex_t *out5, vertex_t *out6, vertex_t *out7,
                               const vertex_t *in1, const vertex_t *in2, const vertex_t *in3, const vertex_t *in4, const vertex_t *in5, const vertex_t *in6, const vertex_t *in7, const vertex_t *in8,
                               const int *s, bool istail)
{
    v8vertex_t R[2];
    v8vertex_pack7(&(R[0]), in1, in2, in3, in4, in5, in6, in7);
    v8vertex_swappack7(&(R[1]), in2, in3, in4, in5, in6, in7, in8);

    v8eval_action_stra(R, s);

    #ifndef NDEBUG
    v8ec_curve_t E1, E2;
    v8A24_to_AC(&E1, &(R[0].A24));
    v8A24_to_AC(&E2, &(R[1].A24));
    fp2_t j11, j12, j13, j14, j15, j16, j17, j21, j22, j23, j24, j25, j26, j27;
    ec_curve_t E11, E12, E13, E14, E15, E16, E17;
    ec_curve_t E21, E22, E23, E24, E25, E26, E27;
    v8fp2_unpack7(&(E11.A), &(E12.A), &(E13.A), &(E14.A), &(E15.A), &(E16.A), &(E17.A), &(E1.A));
    v8fp2_unpack7(&(E21.A), &(E22.A), &(E23.A), &(E24.A), &(E25.A), &(E26.A), &(E27.A), &(E2.A));
    v8fp2_unpack7(&(E11.C), &(E12.C), &(E13.C), &(E14.C), &(E15.C), &(E16.C), &(E17.C), &(E1.C));
    v8fp2_unpack7(&(E21.C), &(E22.C), &(E23.C), &(E24.C), &(E25.C), &(E26.C), &(E27.C), &(E2.C));
    ec_j_inv(&j11, &E11); ec_j_inv(&j21, &E21);
    ec_j_inv(&j12, &E12); ec_j_inv(&j22, &E22);
    ec_j_inv(&j13, &E13); ec_j_inv(&j23, &E23);
    ec_j_inv(&j14, &E14); ec_j_inv(&j24, &E24);
    ec_j_inv(&j15, &E15); ec_j_inv(&j25, &E25);
    ec_j_inv(&j16, &E16); ec_j_inv(&j26, &E26);
    ec_j_inv(&j17, &E17); ec_j_inv(&j27, &E27);
    assert(fp2_is_equal(&j11, &j21));
    assert(fp2_is_equal(&j12, &j22));
    assert(fp2_is_equal(&j13, &j23));
    assert(fp2_is_equal(&j14, &j24));
    assert(fp2_is_equal(&j15, &j25));
    assert(fp2_is_equal(&j16, &j26));
    assert(fp2_is_equal(&j17, &j27));
    #endif
    v8vertex_output7(out1, out2, out3, out4, out5, out6, out7, &(R[0]), &(R[1]));
    
    if (istail)
    {
        ec_point_t t1, t2, t3, t4, t5, t6, t7;
        v8ec_point_unpack7(&t1, &t2, &t3, &t4, &t5, &t6, &t7, &(R[1].A24));
        ec_isom_t correction;
        ec_curve_t head, tail;
        A24_to_AC(&head, &(t7));
        A24_to_AC(&tail, &(out7->A24));
        ec_isomorphism(&correction, &head, &tail);
        ec_iso_eval(&(out7->Ps), &correction);
        ec_iso_eval(&(out7->Pt), &correction);
    }
}

void eval_action_vertex_stra_6(vertex_t *out1, vertex_t *out2, vertex_t *out3, vertex_t *out4, vertex_t *out5, vertex_t *out6,
                               const vertex_t *in1, const vertex_t *in2, const vertex_t *in3, const vertex_t *in4, const vertex_t *in5, const vertex_t *in6, const vertex_t *in7,
                               const int *s, bool istail)
{
    v8vertex_t R[2];
    v8vertex_pack6(&(R[0]), in1, in2, in3, in4, in5, in6);
    v8vertex_swappack6(&(R[1]), in2, in3, in4, in5, in6, in7);

    v8eval_action_stra(R, s);

    #ifndef NDEBUG
    v8ec_curve_t E1, E2;
    v8A24_to_AC(&E1, &(R[0].A24));
    v8A24_to_AC(&E2, &(R[1].A24));
    fp2_t j11, j12, j13, j14, j15, j16, j21, j22, j23, j24, j25, j26;
    ec_curve_t E11, E12, E13, E14, E15, E16;
    ec_curve_t E21, E22, E23, E24, E25, E26;
    v8fp2_unpack6(&(E11.A), &(E12.A), &(E13.A), &(E14.A), &(E15.A), &(E16.A), &(E1.A));
    v8fp2_unpack6(&(E21.A), &(E22.A), &(E23.A), &(E24.A), &(E25.A), &(E26.A), &(E2.A));
    v8fp2_unpack6(&(E11.C), &(E12.C), &(E13.C), &(E14.C), &(E15.C), &(E16.C), &(E1.C));
    v8fp2_unpack6(&(E21.C), &(E22.C), &(E23.C), &(E24.C), &(E25.C), &(E26.C), &(E2.C));
    ec_j_inv(&j11, &E11); ec_j_inv(&j21, &E21);
    ec_j_inv(&j12, &E12); ec_j_inv(&j22, &E22);
    ec_j_inv(&j13, &E13); ec_j_inv(&j23, &E23);
    ec_j_inv(&j14, &E14); ec_j_inv(&j24, &E24);
    ec_j_inv(&j15, &E15); ec_j_inv(&j25, &E25);
    ec_j_inv(&j16, &E16); ec_j_inv(&j26, &E26);
    assert(fp2_is_equal(&j11, &j21));
    assert(fp2_is_equal(&j12, &j22));
    assert(fp2_is_equal(&j13, &j23));
    assert(fp2_is_equal(&j14, &j24));
    assert(fp2_is_equal(&j15, &j25));
    assert(fp2_is_equal(&j16, &j26));
    #endif
    v8vertex_output6(out1, out2, out3, out4, out5, out6, &(R[0]), &(R[1]));
    
    if(istail)
    {
        ec_point_t t1, t2, t3, t4, t5, t6;
        v8ec_point_unpack6(&t1, &t2, &t3, &t4, &t5, &t6, &(R[1].A24));
        ec_isom_t correction;
        ec_curve_t head, tail;
        A24_to_AC(&head, &(t6));
        A24_to_AC(&tail, &(out6->A24));
        ec_isomorphism(&correction, &head, &tail);
        ec_iso_eval(&(out6->Ps), &correction);
        ec_iso_eval(&(out6->Pt), &correction);
    }
    
}

void eval_action_vertex_stra_5(vertex_t *out1, vertex_t *out2, vertex_t *out3, vertex_t *out4, vertex_t *out5,
                               const vertex_t *in1, const vertex_t *in2, const vertex_t *in3, const vertex_t *in4, const vertex_t *in5, const vertex_t *in6,
                               const int *s, bool istail)
{
    v8vertex_t R[2];
    v8vertex_pack5(&(R[0]), in1, in2, in3, in4, in5);
    v8vertex_swappack5(&(R[1]), in2, in3, in4, in5, in6);

    v8eval_action_stra(R, s);

    #ifndef NDEBUG
    v8ec_curve_t E1, E2;
    v8A24_to_AC(&E1, &(R[0].A24));
    v8A24_to_AC(&E2, &(R[1].A24));
    fp2_t j11, j12, j13, j14, j15, j21, j22, j23, j24, j25;
    ec_curve_t E11, E12, E13, E14, E15;
    ec_curve_t E21, E22, E23, E24, E25;
    v8fp2_unpack5(&(E11.A), &(E12.A), &(E13.A), &(E14.A), &(E15.A), &(E1.A));
    v8fp2_unpack5(&(E21.A), &(E22.A), &(E23.A), &(E24.A), &(E25.A), &(E2.A));
    v8fp2_unpack5(&(E11.C), &(E12.C), &(E13.C), &(E14.C), &(E15.C), &(E1.C));
    v8fp2_unpack5(&(E21.C), &(E22.C), &(E23.C), &(E24.C), &(E25.C), &(E2.C));
    ec_j_inv(&j11, &E11); ec_j_inv(&j21, &E21);
    ec_j_inv(&j12, &E12); ec_j_inv(&j22, &E22);
    ec_j_inv(&j13, &E13); ec_j_inv(&j23, &E23);
    ec_j_inv(&j14, &E14); ec_j_inv(&j24, &E24);
    ec_j_inv(&j15, &E15); ec_j_inv(&j25, &E25);
    assert(fp2_is_equal(&j11, &j21));
    assert(fp2_is_equal(&j12, &j22));
    assert(fp2_is_equal(&j13, &j23));
    assert(fp2_is_equal(&j14, &j24));
    assert(fp2_is_equal(&j15, &j25));
    #endif
    v8vertex_output5(out1, out2, out3, out4, out5, &(R[0]), &(R[1]));
    
    if(istail)
    {
        ec_point_t t1, t2, t3, t4, t5;
        v8ec_point_unpack5(&t1, &t2, &t3, &t4, &t5, &(R[1].A24));
        ec_isom_t correction;
        ec_curve_t head, tail;
        A24_to_AC(&head, &(t5));
        A24_to_AC(&tail, &(out5->A24));
        ec_isomorphism(&correction, &head, &tail);
        ec_iso_eval(&(out5->Ps), &correction);
        ec_iso_eval(&(out5->Pt), &correction);
    }
    
}

void eval_action_vertex_stra_4(vertex_t *out1, vertex_t *out2, vertex_t *out3, vertex_t *out4,
                               const vertex_t *in1, const vertex_t *in2, const vertex_t *in3, const vertex_t *in4, const vertex_t *in5,
                               const int *s, bool istail)
{
    v8vertex_t R[2];
    v8vertex_pack4(&(R[0]), in1, in2, in3, in4);
    v8vertex_swappack4(&(R[1]), in2, in3, in4, in5);

    v8eval_action_stra(R, s);

    #ifndef NDEBUG
    v8ec_curve_t E1, E2;
    v8A24_to_AC(&E1, &(R[0].A24));
    v8A24_to_AC(&E2, &(R[1].A24));
    fp2_t j11, j12, j13, j14, j21, j22, j23, j24;
    ec_curve_t E11, E12, E13, E14;
    ec_curve_t E21, E22, E23, E24;
    v8fp2_unpack4(&(E11.A), &(E12.A), &(E13.A), &(E14.A), &(E1.A));
    v8fp2_unpack4(&(E21.A), &(E22.A), &(E23.A), &(E24.A), &(E2.A));
    v8fp2_unpack4(&(E11.C), &(E12.C), &(E13.C), &(E14.C), &(E1.C));
    v8fp2_unpack4(&(E21.C), &(E22.C), &(E23.C), &(E24.C), &(E2.C));
    ec_j_inv(&j11, &E11); ec_j_inv(&j21, &E21);
    ec_j_inv(&j12, &E12); ec_j_inv(&j22, &E22);
    ec_j_inv(&j13, &E13); ec_j_inv(&j23, &E23);
    ec_j_inv(&j14, &E14); ec_j_inv(&j24, &E24);
    assert(fp2_is_equal(&j11, &j21));
    assert(fp2_is_equal(&j12, &j22));
    assert(fp2_is_equal(&j13, &j23));
    assert(fp2_is_equal(&j14, &j24));
    #endif
    v8vertex_output4(out1, out2, out3, out4, &(R[0]), &(R[1]));
    
    if(istail)
    {
        ec_point_t t1, t2, t3, t4;
        v8ec_point_unpack4(&t1, &t2, &t3, &t4, &(R[1].A24));
        ec_isom_t correction;
        ec_curve_t head, tail;
        A24_to_AC(&head, &(t4));
        A24_to_AC(&tail, &(out4->A24));
        ec_isomorphism(&correction, &head, &tail);
        ec_iso_eval(&(out4->Ps), &correction);
        ec_iso_eval(&(out4->Pt), &correction);
    }
    
}

void eval_action_vertex_stra_3(vertex_t *out1, vertex_t *out2, vertex_t *out3,
                               const vertex_t *in1, const vertex_t *in2, const vertex_t *in3, const vertex_t *in4,
                               const int *s, bool istail)
{
    v8vertex_t R[2];
    v8vertex_pack3(&(R[0]), in1, in2, in3);
    v8vertex_swappack3(&(R[1]), in2, in3, in4);

    v8eval_action_stra(R, s);

    #ifndef NDEBUG
    v8ec_curve_t E1, E2;
    v8A24_to_AC(&E1, &(R[0].A24));
    v8A24_to_AC(&E2, &(R[1].A24));
    fp2_t j11, j12, j13, j21, j22, j23;
    ec_curve_t E11, E12, E13;
    ec_curve_t E21, E22, E23;
    v8fp2_unpack3(&(E11.A), &(E12.A), &(E13.A), &(E1.A));
    v8fp2_unpack3(&(E21.A), &(E22.A), &(E23.A), &(E2.A));
    v8fp2_unpack3(&(E11.C), &(E12.C), &(E13.C), &(E1.C));
    v8fp2_unpack3(&(E21.C), &(E22.C), &(E23.C), &(E2.C));
    ec_j_inv(&j11, &E11); ec_j_inv(&j21, &E21);
    ec_j_inv(&j12, &E12); ec_j_inv(&j22, &E22);
    ec_j_inv(&j13, &E13); ec_j_inv(&j23, &E23);
    assert(fp2_is_equal(&j11, &j21));
    assert(fp2_is_equal(&j12, &j22));
    assert(fp2_is_equal(&j13, &j23));
    #endif
    v8vertex_output3(out1, out2, out3, &(R[0]), &(R[1]));
    
    if(istail)
    {
        ec_point_t t1, t2, t3;
        v8ec_point_unpack3(&t1, &t2, &t3, &(R[1].A24));
        ec_isom_t correction;
        ec_curve_t head, tail;
        A24_to_AC(&head, &(t3));
        A24_to_AC(&tail, &(out3->A24));
        ec_isomorphism(&correction, &head, &tail);
        ec_iso_eval(&(out3->Ps), &correction);
        ec_iso_eval(&(out3->Pt), &correction);
    }
    
}

void eval_action_vertex_stra_2(vertex_t *out1, vertex_t *out2,
                               const vertex_t *in1, const vertex_t *in2, const vertex_t *in3,
                               const int *s, bool istail)
{
    v8vertex_t R[2];
    v8vertex_pack2(&(R[0]), in1, in2);
    v8vertex_swappack2(&(R[1]), in2, in3);

    v8eval_action_stra(R, s);

    #ifndef NDEBUG
    v8ec_curve_t E1, E2;
    v8A24_to_AC(&E1, &(R[0].A24));
    v8A24_to_AC(&E2, &(R[1].A24));
    fp2_t j11, j12, j21, j22;
    ec_curve_t E11, E12;
    ec_curve_t E21, E22;
    v8fp2_unpack2(&(E11.A), &(E12.A), &(E1.A));
    v8fp2_unpack2(&(E21.A), &(E22.A), &(E2.A));
    v8fp2_unpack2(&(E11.C), &(E12.C), &(E1.C));
    v8fp2_unpack2(&(E21.C), &(E22.C), &(E2.C));
    ec_j_inv(&j11, &E11); ec_j_inv(&j21, &E21);
    ec_j_inv(&j12, &E12); ec_j_inv(&j22, &E22);
    assert(fp2_is_equal(&j11, &j21));
    assert(fp2_is_equal(&j12, &j22));
    #endif
    v8vertex_output2(out1, out2, &(R[0]), &(R[1]));
    
    if(istail)
    {
        ec_point_t t1, t2;
        v8ec_point_unpack2(&t1, &t2, &(R[1].A24));
        ec_isom_t correction;
        ec_curve_t head, tail;
        A24_to_AC(&head, &(t2));
        A24_to_AC(&tail, &(out2->A24));
        ec_isomorphism(&correction, &head, &tail);
        ec_iso_eval(&(out2->Ps), &correction);
        ec_iso_eval(&(out2->Pt), &correction);
    }
    
}

void eval_action_vertex_stra_8_split(vertex_t *out1, vertex_t *out2, vertex_t *out3, vertex_t *out4, vertex_t *out5, vertex_t *out6, vertex_t *out7, vertex_t *out8,
                               const vertex_t *in1, const vertex_t *in2, const vertex_t *in3, const vertex_t *in4, const vertex_t *in5, const vertex_t *in6, const vertex_t *in7, const vertex_t *in8, const vertex_t *in9, const vertex_t *in10,
                               const int *s1, const int *s2, SPLIT split, bool istail)
{
    v8vertex_t R[2];
    v8vertex_pack_split(&(R[0]), &(R[1]), in1, in2, in3, in4, in5, in6, in7, in8, in9, in10, split);
    
    int strascalar_i;
    v8ec_point_t B24;

    // Isogenies with kernel in E[p+1]
    strascalar_i = 0;
    v8ec_point_t stack_kernel[2][stack_volume];
    int stack_scalar[stack_volume];
    size_t top = 0;
    v8copy_point(&(stack_kernel[0][top]), &(R[0].Ps));
    v8copy_point(&(stack_kernel[1][top]), &(R[1].Ps));
    stack_scalar[top] = 0;
    top++;
    int jj = 0, k = 0;
    for(int i = 0; i < P_LEN; i++)
    {
        for(int t = 1; t <= TORSION_ODD_POWERS[i]; t++)
        {
            __m512i b = v8compare_gt_split(t, s1[i], t, s2[i], split);
            jj++;
            for (int si = 0; si < top; si++)
            {
                v8swap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b);
            }
            v8swap_vertices(&(R[0]), &(R[1]), b);
            while(stack_scalar[top-1] < degree_s_num - jj)
            {
                stack_scalar[top] = stack_scalar[top-1] + strategy_s[k];
                // v8xMULv2(&(stack_kernel[0][top]), &(stack_kernel[0][top-1]), &(strascalar_s[strascalar_i]), strascalar_s_bits[k], &(R[0].A24));
                // if (stack_scalar[top] < degree_s_num - jj) // This conditional check does NOT leak any secret information
                // {
                //     v8xMULv2(&(stack_kernel[1][top]), &(stack_kernel[1][top-1]), &(strascalar_s[strascalar_i]), strascalar_s_bits[k], &(R[1].A24));
                // }
                // strascalar_i += strascalar_s_limbs[k];
                v8stra_scalar_func_s[strascalar_i](&(stack_kernel[0][top]), &(stack_kernel[0][top-1]), &(R[0].A24));
                if (stack_scalar[top] < degree_s_num - jj) // This conditional check does NOT leak any secret information
                {
                    v8stra_scalar_func_s[strascalar_i](&(stack_kernel[1][top]), &(stack_kernel[1][top-1]), &(R[1].A24));
                }
                strascalar_i++;
                top++;
                k++;
                assert(top <= stack_volume);
            }
            top--;
            assert(top >= 0);
            v8kps(i, &(stack_kernel[0][top]), &(R[0].A24));
            v8xisog(&B24, i, &(R[0].A24));
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                v8xeval(&(stack_kernel[0][traverse_stack]), i, &(stack_kernel[0][traverse_stack]), &(R[0].A24));
            }
            v8xeval(&(R[0].Pt), i, &(R[0].Pt), &(R[0].A24));
            v8xeval(&(R[0].Qs), i, &(R[0].Qs), &(R[0].A24));
            v8xeval(&(R[0].Qt), i, &(R[0].Qt), &(R[0].A24));
            v8copy_point(&(R[0].A24), &B24);
            v8kps_clear(i);
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                // v8xMULv2(&(stack_kernel[1][traverse_stack]), &(stack_kernel[1][traverse_stack]), &(TORSION_ODD_PRIMES[i]), p_plus_minus_bitlength[i], &(R[1].A24));
                v8torsionscalar_func[i](&(stack_kernel[1][traverse_stack]), &(stack_kernel[1][traverse_stack]), &(R[1].A24));
            }
            for (int si = 0; si < top; si++)
            {
                v8swap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b);
            }
            v8swap_vertices(&(R[0]), &(R[1]), b);
        }
    }
    assert(top == 0);

    // Isogenies with kernel in E[p-1]
    strascalar_i = 0;
    top = 0;
    v8copy_point(&(stack_kernel[0][top]), &(R[0].Pt));
    v8copy_point(&(stack_kernel[1][top]), &(R[1].Pt));
    stack_scalar[top] = 0;
    top++;
    jj = 0; k = 0;
    for(int i = P_LEN; i < P_LEN + M_LEN; i++)
    {
        for(int t = 1; t <= TORSION_ODD_POWERS[i]; t++)
        {
            __m512i b = v8compare_gt_split(t, s1[i], t, s2[i], split);
            jj++;
            for (int si = 0; si < top; si++)
            {
                v8swap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b);
            }
            v8swap_vertices(&(R[0]), &(R[1]), b);
            while(stack_scalar[top-1] < degree_t_num - jj)
            {
                stack_scalar[top] = stack_scalar[top-1] + strategy_t[k];
                // v8xMULv2(&(stack_kernel[0][top]), &(stack_kernel[0][top-1]), &(strascalar_t[strascalar_i]), strascalar_t_bits[k], &(R[0].A24));
                // if (stack_scalar[top] < degree_t_num - jj) // This conditional check does NOT leak any secret information
                // {
                //     v8xMULv2(&(stack_kernel[1][top]), &(stack_kernel[1][top-1]), &(strascalar_t[strascalar_i]), strascalar_t_bits[k], &(R[1].A24));
                // }
                // strascalar_i += strascalar_t_limbs[k];
                v8stra_scalar_func_t[strascalar_i](&(stack_kernel[0][top]), &(stack_kernel[0][top-1]), &(R[0].A24));
                if (stack_scalar[top] < degree_t_num - jj) // This conditional check does NOT leak any secret information
                {
                    v8stra_scalar_func_t[strascalar_i](&(stack_kernel[1][top]), &(stack_kernel[1][top-1]), &(R[1].A24));
                }
                strascalar_i++;
                top++;
                k++;
                assert(top <= stack_volume);
            }
            top--;
            assert(top >= 0);
            v8kps(i, &(stack_kernel[0][top]), &(R[0].A24));
            v8xisog(&B24, i, &(R[0].A24));
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                v8xeval(&(stack_kernel[0][traverse_stack]), i, &(stack_kernel[0][traverse_stack]), &(R[0].A24));
            }
            // v8xeval(&(R[b].Ps), i, R[b].Ps, R[b].A24);
            v8xeval(&(R[0].Qs), i, &(R[0].Qs), &(R[0].A24));
            v8xeval(&(R[0].Qt), i, &(R[0].Qt), &(R[0].A24));
            v8copy_point(&(R[0].A24), &B24);
            v8kps_clear(i);
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                // v8xMULv2(&(stack_kernel[1][traverse_stack]), &(stack_kernel[1][traverse_stack]), &(TORSION_ODD_PRIMES[i]), p_plus_minus_bitlength[i], &(R[1].A24));
                v8torsionscalar_func[i](&(stack_kernel[1][traverse_stack]), &(stack_kernel[1][traverse_stack]), &(R[1].A24));
            }
            for (int si = 0; si < top; si++)
            {
                v8swap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b);
            }
            v8swap_vertices(&(R[0]), &(R[1]), b);
        }
    }
    assert(top == 0);

    #ifndef NDEBUG
    v8ec_curve_t E1, E2;
    v8A24_to_AC(&E1, &(R[0].A24));
    v8A24_to_AC(&E2, &(R[1].A24));
    fp2_t j11, j12, j13, j14, j15, j16, j17, j18, j21, j22, j23, j24, j25, j26, j27, j28;
    ec_curve_t E11, E12, E13, E14, E15, E16, E17, E18;
    ec_curve_t E21, E22, E23, E24, E25, E26, E27, E28;
    v8fp2_unpack8(&(E11.A), &(E12.A), &(E13.A), &(E14.A), &(E15.A), &(E16.A), &(E17.A), &(E18.A), &(E1.A));
    v8fp2_unpack8(&(E21.A), &(E22.A), &(E23.A), &(E24.A), &(E25.A), &(E26.A), &(E27.A), &(E28.A), &(E2.A));
    v8fp2_unpack8(&(E11.C), &(E12.C), &(E13.C), &(E14.C), &(E15.C), &(E16.C), &(E17.C), &(E18.C), &(E1.C));
    v8fp2_unpack8(&(E21.C), &(E22.C), &(E23.C), &(E24.C), &(E25.C), &(E26.C), &(E27.C), &(E28.C), &(E2.C));
    ec_j_inv(&j11, &E11); ec_j_inv(&j21, &E21);
    ec_j_inv(&j12, &E12); ec_j_inv(&j22, &E22);
    ec_j_inv(&j13, &E13); ec_j_inv(&j23, &E23);
    ec_j_inv(&j14, &E14); ec_j_inv(&j24, &E24);
    ec_j_inv(&j15, &E15); ec_j_inv(&j25, &E25);
    ec_j_inv(&j16, &E16); ec_j_inv(&j26, &E26);
    ec_j_inv(&j17, &E17); ec_j_inv(&j27, &E27);
    ec_j_inv(&j18, &E18); ec_j_inv(&j28, &E28);
    assert(fp2_is_equal(&j11, &j21));
    assert(fp2_is_equal(&j12, &j22));
    assert(fp2_is_equal(&j13, &j23));
    assert(fp2_is_equal(&j14, &j24));
    assert(fp2_is_equal(&j15, &j25));
    assert(fp2_is_equal(&j16, &j26));
    assert(fp2_is_equal(&j17, &j27));
    assert(fp2_is_equal(&j18, &j28));
    #endif

    v8vertex_output8(out1, out2, out3, out4, out5, out6, out7, out8, &(R[0]), &(R[1]));
    if (istail)
    {
        ec_point_t t1, t2, t3, t4, t5, t6, t7, t8;
        v8ec_point_unpack8(&t1, &t2, &t3, &t4, &t5, &t6, &t7, &t8, &(R[1].A24));
        vertex_t *headvertex;
        ec_point_t *headA24, *tailA24;
        switch (split)
        {
        case ONE:
            headvertex = out1;
            tailA24 = &t1;
            break;
        case TWO:
            headvertex = out2;
            tailA24 = &t2;
            break;
        case THREE:
            headvertex = out3;
            tailA24 = &t3;
            break;
        case FOUR:
            headvertex = out4;
            tailA24 = &t4;
            break;
        case FIVE:
            headvertex = out5;
            tailA24 = &t5;
            break;
        case SIX:
            headvertex = out6;
            tailA24 = &t6;
            break;
        case SEVEN:
            headvertex = out7;
            tailA24 = &t7;
            break;
        default:
            assert(0);
        }

        ec_isom_t correction;
        ec_curve_t head, tail;
        A24_to_AC(&head, tailA24);
        A24_to_AC(&tail, &(headvertex->A24));
        ec_isomorphism(&correction, &head, &tail);
        ec_iso_eval(&(headvertex->Ps), &correction);
        ec_iso_eval(&(headvertex->Pt), &correction);
    }
}

void niike_PublicKey(orient_cycle_t* out, secretkey_t s)
{
    secretkey_t action[ITERATIONS];
    orient_cycle_t iter[ITERATIONS - 1];
    memcpy(action[0], s, sizeof(secretkey_t));
    for(int i = 1; i < ITERATIONS; i++)
    {
        for(int j = 0; j < P_LEN + M_LEN; j++)
        {
            action[i][j] = action[i - 1][j] - TORSION_ODD_POWERS[j];
        }
    }
    eval_action_vertex_stra_8(&(iter[0][0]), &(iter[0][1]), &(iter[0][2]), &(iter[0][3]), &(iter[0][4]), &(iter[0][5]), &(iter[0][6]), &(iter[0][7]),
                              &(base_cycle[0]), &(base_cycle[1]), &(base_cycle[2]), &(base_cycle[3]), &(base_cycle[4]), &(base_cycle[5]), &(base_cycle[6]), &(base_cycle[7]), &(base_cycle[8]),
                              action[0], false);
    eval_action_vertex_stra_8_split(&(iter[0][8]), &(iter[0][9]), &(iter[0][10]), &(iter[0][11]), &(iter[0][12]), &(iter[1][0]), &(iter[1][1]), &(iter[1][2]),
                                    &(base_cycle[8]), &(base_cycle[9]), &(base_cycle[10]), &(base_cycle[11]), &(base_cycle[12]), &(base_cycle[0]), &(iter[0][0]), &(iter[0][1]), &(iter[0][2]), &(iter[0][3]),
                                    action[0], action[1], FIVE, true);
    eval_action_vertex_stra_8(&(iter[1][3]), &(iter[1][4]), &(iter[1][5]), &(iter[1][6]), &(iter[1][7]), &(iter[1][8]), &(iter[1][9]), &(iter[1][10]),
                              &(iter[0][3]), &(iter[0][4]), &(iter[0][5]), &(iter[0][6]), &(iter[0][7]), &(iter[0][8]), &(iter[0][9]), &(iter[0][10]), &(iter[0][11]),
                              action[1], false);
    eval_action_vertex_stra_8_split(&(iter[1][11]), &(iter[1][12]), &(iter[2][0]), &(iter[2][1]), &(iter[2][2]), &(iter[2][3]), &(iter[2][4]), &(iter[2][5]),
                                    &(iter[0][11]), &(iter[0][12]), &(iter[0][0]), &(iter[1][0]), &(iter[1][1]), &(iter[1][2]), &(iter[1][3]), &(iter[1][4]), &(iter[1][5]), &(iter[1][6]),
                                    action[1], action[2], TWO, true);
    eval_action_vertex_stra_8_split(&(iter[2][6]), &(iter[2][7]), &(iter[2][8]), &(iter[2][9]), &(iter[2][10]), &(iter[2][11]), &(iter[2][12]), &(iter[3][0]),
                                    &(iter[1][6]), &(iter[1][7]), &(iter[1][8]), &(iter[1][9]), &(iter[1][10]), &(iter[1][11]), &(iter[1][12]), &(iter[1][0]), &(iter[2][0]), &(iter[2][1]),
                                    action[2], action[3], SEVEN, true);
    eval_action_vertex_stra_8(&(iter[3][1]), &(iter[3][2]), &(iter[3][3]), &(iter[3][4]), &(iter[3][5]), &(iter[3][6]), &(iter[3][7]), &(iter[3][8]),
                              &(iter[2][1]), &(iter[2][2]), &(iter[2][3]), &(iter[2][4]), &(iter[2][5]), &(iter[2][6]), &(iter[2][7]), &(iter[2][8]), &(iter[2][9]),
                              action[3], false);
    eval_action_vertex_stra_8_split(&(iter[3][9]), &(iter[3][10]), &(iter[3][11]), &(iter[3][12]), &(iter[4][0]), &(iter[4][1]), &(iter[4][2]), &(iter[4][3]),
                                    &(iter[2][9]), &(iter[2][10]), &(iter[2][11]), &(iter[2][12]), &(iter[2][0]), &(iter[3][0]), &(iter[3][1]), &(iter[3][2]), &(iter[3][3]), &(iter[3][4]),
                                    action[3], action[4], FOUR, true);
    eval_action_vertex_stra_8(&(iter[4][4]), &(iter[4][5]), &(iter[4][6]), &(iter[4][7]), &(iter[4][8]), &(iter[4][9]), &(iter[4][10]), &(iter[4][11]),
                              &(iter[3][4]), &(iter[3][5]), &(iter[3][6]), &(iter[3][7]), &(iter[3][8]), &(iter[3][9]), &(iter[3][10]), &(iter[3][11]), &(iter[3][12]),
                              action[4], false);
    eval_action_vertex_stra_8_split(&(iter[4][12]), &(iter[5][0]), &(iter[5][1]), &(iter[5][2]), &(iter[5][3]), &(iter[5][4]), &(iter[5][5]), &(iter[5][6]),
                                    &(iter[3][12]), &(iter[3][0]), &(iter[4][0]), &(iter[4][1]), &(iter[4][2]), &(iter[4][3]), &(iter[4][4]), &(iter[4][5]), &(iter[4][6]), &(iter[4][7]),
                                    action[4], action[5], ONE, true);
    eval_action_vertex_stra_8_split(&(iter[5][7]), &(iter[5][8]), &(iter[5][9]), &(iter[5][10]), &(iter[5][11]), &(iter[5][12]), &(iter[6][0]), &(iter[6][1]),
                                    &(iter[4][7]), &(iter[4][8]), &(iter[4][9]), &(iter[4][10]), &(iter[4][11]), &(iter[4][12]), &(iter[4][0]), &(iter[5][0]), &(iter[5][1]), &(iter[5][2]),
                                    action[5], action[6], SIX, true);
    eval_action_vertex_stra_8(&(iter[6][2]), &(iter[6][3]), &(iter[6][4]), &(iter[6][5]), &(iter[6][6]), &(iter[6][7]), &(iter[6][8]), &(iter[6][9]),
                              &(iter[5][2]), &(iter[5][3]), &(iter[5][4]), &(iter[5][5]), &(iter[5][6]), &(iter[5][7]), &(iter[5][8]), &(iter[5][9]), &(iter[5][10]),
                              action[6], false);
    eval_action_vertex_stra_8_split(&(iter[6][10]), &(iter[6][11]), &(iter[6][12]), &(iter[7][0]), &(iter[7][1]), &(iter[7][2]), &(iter[7][3]), &(iter[7][4]),
                                    &(iter[5][10]), &(iter[5][11]), &(iter[5][12]), &(iter[5][0]), &(iter[6][0]), &(iter[6][1]), &(iter[6][2]), &(iter[6][3]), &(iter[6][4]), &(iter[6][5]),
                                    action[6], action[7], THREE, true);
    eval_action_vertex_stra_8(&(iter[7][5]), &(iter[7][6]), &(iter[7][7]), &(iter[7][8]), &(iter[7][9]), &(iter[7][10]), &(iter[7][11]), &(iter[7][12]),
                              &(iter[6][5]), &(iter[6][6]), &(iter[6][7]), &(iter[6][8]), &(iter[6][9]), &(iter[6][10]), &(iter[6][11]), &(iter[6][12]), &(iter[6][0]),
                              action[7], true);
    eval_action_vertex_stra_8(&(iter[8][0]), &(iter[8][1]), &(iter[8][2]), &(iter[8][3]), &(iter[8][4]), &(iter[8][5]), &(iter[8][6]), &(iter[8][7]),
                              &(iter[7][0]), &(iter[7][1]), &(iter[7][2]), &(iter[7][3]), &(iter[7][4]), &(iter[7][5]), &(iter[7][6]), &(iter[7][7]), &(iter[7][8]),
                              action[8], false);
    eval_action_vertex_stra_8_split(&(iter[8][8]), &(iter[8][9]), &(iter[8][10]), &(iter[8][11]), &(iter[8][12]), &(iter[9][0]), &(iter[9][1]), &(iter[9][2]),
                                    &(iter[7][8]), &(iter[7][9]), &(iter[7][10]), &(iter[7][11]), &(iter[7][12]), &(iter[7][0]), &(iter[8][0]), &(iter[8][1]), &(iter[8][2]), &(iter[8][3]),
                                    action[8], action[9], FIVE, true);
    eval_action_vertex_stra_8(&(iter[9][3]), &(iter[9][4]), &(iter[9][5]), &(iter[9][6]), &(iter[9][7]), &(iter[9][8]), &(iter[9][9]), &(iter[9][10]),
                              &(iter[8][3]), &(iter[8][4]), &(iter[8][5]), &(iter[8][6]), &(iter[8][7]), &(iter[8][8]), &(iter[8][9]), &(iter[8][10]), &(iter[8][11]),
                              action[9], false);
    eval_action_vertex_stra_8_split(&(iter[9][11]), &(iter[9][12]), &(iter[10][0]), &(iter[10][1]), &(iter[10][2]), &(iter[10][3]), &(iter[10][4]), &(iter[10][5]),
                                    &(iter[8][11]), &(iter[8][12]), &(iter[8][0]), &(iter[9][0]), &(iter[9][1]), &(iter[9][2]), &(iter[9][3]), &(iter[9][4]), &(iter[9][5]), &(iter[9][6]),
                                    action[9], action[10], TWO, true);
    eval_action_vertex_stra_8_split(&(iter[10][6]), &(iter[10][7]), &(iter[10][8]), &(iter[10][9]), &(iter[10][10]), &(iter[10][11]), &(iter[10][12]), &(iter[11][0]),
                                    &(iter[9][6]), &(iter[9][7]), &(iter[9][8]), &(iter[9][9]), &(iter[9][10]), &(iter[9][11]), &(iter[9][12]), &(iter[9][0]), &(iter[10][0]), &(iter[10][1]),
                                    action[10], action[11], SEVEN, true);
    eval_action_vertex_stra_8(&(iter[11][1]), &(iter[11][2]), &(iter[11][3]), &(iter[11][4]), &(iter[11][5]), &(iter[11][6]), &(iter[11][7]), &(iter[11][8]),
                              &(iter[10][1]), &(iter[10][2]), &(iter[10][3]), &(iter[10][4]), &(iter[10][5]), &(iter[10][6]), &(iter[10][7]), &(iter[10][8]), &(iter[10][9]),
                              action[11], false);
    eval_action_vertex_stra_8_split(&(iter[11][9]), &(iter[11][10]), &(iter[11][11]), &(iter[11][12]), &(iter[12][0]), &(iter[12][1]), &(iter[12][2]), &(iter[12][3]),
                                    &(iter[10][9]), &(iter[10][10]), &(iter[10][11]), &(iter[10][12]), &(iter[10][0]), &(iter[11][0]), &(iter[11][1]), &(iter[11][2]), &(iter[11][3]), &(iter[11][4]),
                                    action[11], action[12], FOUR, true);
    eval_action_vertex_stra_8(&(iter[12][4]), &(iter[12][5]), &(iter[12][6]), &(iter[12][7]), &(iter[12][8]), &(iter[12][9]), &(iter[12][10]), &(iter[12][11]),
                              &(iter[11][4]), &(iter[11][5]), &(iter[11][6]), &(iter[11][7]), &(iter[11][8]), &(iter[11][9]), &(iter[11][10]), &(iter[11][11]), &(iter[11][12]),
                              action[12], false);
    eval_action_vertex_stra_8_split(&(iter[12][12]), &(iter[13][0]), &(iter[13][1]), &(iter[13][2]), &(iter[13][3]), &(iter[13][4]), &(iter[13][5]), &(iter[13][6]),
                                    &(iter[11][12]), &(iter[11][0]), &(iter[12][0]), &(iter[12][1]), &(iter[12][2]), &(iter[12][3]), &(iter[12][4]), &(iter[12][5]), &(iter[12][6]), &(iter[12][7]),
                                    action[12], action[13], ONE, true);
    eval_action_vertex_stra_8_split(&(iter[13][7]), &(iter[13][8]), &(iter[13][9]), &(iter[13][10]), &(iter[13][11]), &(iter[13][12]), &(iter[14][0]), &(iter[14][1]),
                                    &(iter[12][7]), &(iter[12][8]), &(iter[12][9]), &(iter[12][10]), &(iter[12][11]), &(iter[12][12]), &(iter[12][0]), &(iter[13][0]), &(iter[13][1]), &(iter[13][2]),
                                    action[13], action[14], SIX, true);
    eval_action_vertex_stra_8(&(iter[14][2]), &(iter[14][3]), &(iter[14][4]), &(iter[14][5]), &(iter[14][6]), &(iter[14][7]), &(iter[14][8]), &(iter[14][9]),
                              &(iter[13][2]), &(iter[13][3]), &(iter[13][4]), &(iter[13][5]), &(iter[13][6]), &(iter[13][7]), &(iter[13][8]), &(iter[13][9]), &(iter[13][10]),
                              action[14], false);
    eval_action_vertex_stra_8_split(&(iter[14][10]), &(iter[14][11]), &(iter[14][12]), &(iter[15][0]), &(iter[15][1]), &(iter[15][2]), &(iter[15][3]), &(iter[15][4]),
                                    &(iter[13][10]), &(iter[13][11]), &(iter[13][12]), &(iter[13][0]), &(iter[14][0]), &(iter[14][1]), &(iter[14][2]), &(iter[14][3]), &(iter[14][4]), &(iter[14][5]),
                                    action[14], action[15], THREE, true);
    eval_action_vertex_stra_8(&(iter[15][5]), &(iter[15][6]), &(iter[15][7]), &(iter[15][8]), &(iter[15][9]), &(iter[15][10]), &(iter[15][11]), &(iter[15][12]),
                              &(iter[14][5]), &(iter[14][6]), &(iter[14][7]), &(iter[14][8]), &(iter[14][9]), &(iter[14][10]), &(iter[14][11]), &(iter[14][12]), &(iter[14][0]),
                              action[15], true);
    eval_action_vertex_stra_8(&(iter[16][0]), &(iter[16][1]), &(iter[16][2]), &(iter[16][3]), &(iter[16][4]), &(iter[16][5]), &(iter[16][6]), &(iter[16][7]),
                              &(iter[15][0]), &(iter[15][1]), &(iter[15][2]), &(iter[15][3]), &(iter[15][4]), &(iter[15][5]), &(iter[15][6]), &(iter[15][7]), &(iter[15][8]),
                              action[16], false);
    eval_action_vertex_stra_8_split(&(iter[16][8]), &(iter[16][9]), &(iter[16][10]), &(iter[16][11]), &(iter[16][12]), &(iter[17][0]), &(iter[17][1]), &(iter[17][2]),
                                    &(iter[15][8]), &(iter[15][9]), &(iter[15][10]), &(iter[15][11]), &(iter[15][12]), &(iter[15][0]), &(iter[16][0]), &(iter[16][1]), &(iter[16][2]), &(iter[16][3]),
                                    action[16], action[17], FIVE, true);
    eval_action_vertex_stra_8(&(iter[17][3]), &(iter[17][4]), &(iter[17][5]), &(iter[17][6]), &(iter[17][7]), &(iter[17][8]), &(iter[17][9]), &(iter[17][10]),
                              &(iter[16][3]), &(iter[16][4]), &(iter[16][5]), &(iter[16][6]), &(iter[16][7]), &(iter[16][8]), &(iter[16][9]), &(iter[16][10]), &(iter[16][11]),
                              action[17], false);
    eval_action_vertex_stra_8_split(&(iter[17][11]), &(iter[17][12]), &(iter[18][0]), &(iter[18][1]), &(iter[18][2]), &(iter[18][3]), &(iter[18][4]), &(iter[18][5]),
                                    &(iter[16][11]), &(iter[16][12]), &(iter[16][0]), &(iter[17][0]), &(iter[17][1]), &(iter[17][2]), &(iter[17][3]), &(iter[17][4]), &(iter[17][5]), &(iter[17][6]),
                                    action[17], action[18], TWO, true);
    eval_action_vertex_stra_8_split(&(iter[18][6]), &(iter[18][7]), &(iter[18][8]), &(iter[18][9]), &(iter[18][10]), &(iter[18][11]), &(iter[18][12]), &(iter[19][0]),
                                    &(iter[17][6]), &(iter[17][7]), &(iter[17][8]), &(iter[17][9]), &(iter[17][10]), &(iter[17][11]), &(iter[17][12]), &(iter[17][0]), &(iter[18][0]), &(iter[18][1]),
                                    action[18], action[19], SEVEN, true);
    eval_action_vertex_stra_8(&(iter[19][1]), &(iter[19][2]), &(iter[19][3]), &(iter[19][4]), &(iter[19][5]), &(iter[19][6]), &(iter[19][7]), &(iter[19][8]),
                              &(iter[18][1]), &(iter[18][2]), &(iter[18][3]), &(iter[18][4]), &(iter[18][5]), &(iter[18][6]), &(iter[18][7]), &(iter[18][8]), &(iter[18][9]),
                              action[19], false);
    eval_action_vertex_stra_8_split(&(iter[19][9]), &(iter[19][10]), &(iter[19][11]), &(iter[19][12]), &(iter[20][0]), &(iter[20][1]), &(iter[20][2]), &(iter[20][3]),
                                    &(iter[18][9]), &(iter[18][10]), &(iter[18][11]), &(iter[18][12]), &(iter[18][0]), &(iter[19][0]), &(iter[19][1]), &(iter[19][2]), &(iter[19][3]), &(iter[19][4]),
                                    action[19], action[20], FOUR, true);
    eval_action_vertex_stra_8(&(iter[20][4]), &(iter[20][5]), &(iter[20][6]), &(iter[20][7]), &(iter[20][8]), &(iter[20][9]), &(iter[20][10]), &(iter[20][11]),
                              &(iter[19][4]), &(iter[19][5]), &(iter[19][6]), &(iter[19][7]), &(iter[19][8]), &(iter[19][9]), &(iter[19][10]), &(iter[19][11]), &(iter[19][12]),
                              action[20], false);
    eval_action_vertex_stra_8_split(&(iter[20][12]), &(iter[21][0]), &(iter[21][1]), &(iter[21][2]), &(iter[21][3]), &(iter[21][4]), &(iter[21][5]), &(iter[21][6]),
                                    &(iter[19][12]), &(iter[19][0]), &(iter[20][0]), &(iter[20][1]), &(iter[20][2]), &(iter[20][3]), &(iter[20][4]), &(iter[20][5]), &(iter[20][6]), &(iter[20][7]),
                                    action[20], action[21], ONE, true);
    eval_action_vertex_stra_8_split(&(iter[21][7]), &(iter[21][8]), &(iter[21][9]), &(iter[21][10]), &(iter[21][11]), &(iter[21][12]), &(iter[22][0]), &(iter[22][1]),
                                    &(iter[20][7]), &(iter[20][8]), &(iter[20][9]), &(iter[20][10]), &(iter[20][11]), &(iter[20][12]), &(iter[20][0]), &(iter[21][0]), &(iter[21][1]), &(iter[21][2]),
                                    action[21], action[22], SIX, true);
    eval_action_vertex_stra_8(&(iter[22][2]), &(iter[22][3]), &(iter[22][4]), &(iter[22][5]), &(iter[22][6]), &(iter[22][7]), &(iter[22][8]), &(iter[22][9]),
                              &(iter[21][2]), &(iter[21][3]), &(iter[21][4]), &(iter[21][5]), &(iter[21][6]), &(iter[21][7]), &(iter[21][8]), &(iter[21][9]), &(iter[21][10]),
                              action[22], false);
    eval_action_vertex_stra_8_split(&(iter[22][10]), &(iter[22][11]), &(iter[22][12]), &(iter[23][0]), &(iter[23][1]), &(iter[23][2]), &(iter[23][3]), &(iter[23][4]),
                                    &(iter[21][10]), &(iter[21][11]), &(iter[21][12]), &(iter[21][0]), &(iter[22][0]), &(iter[22][1]), &(iter[22][2]), &(iter[22][3]), &(iter[22][4]), &(iter[22][5]),
                                    action[22], action[23], THREE, true);
    eval_action_vertex_stra_8(&(iter[23][5]), &(iter[23][6]), &(iter[23][7]), &(iter[23][8]), &(iter[23][9]), &(iter[23][10]), &(iter[23][11]), &(iter[23][12]),
                              &(iter[22][5]), &(iter[22][6]), &(iter[22][7]), &(iter[22][8]), &(iter[22][9]), &(iter[22][10]), &(iter[22][11]), &(iter[22][12]), &(iter[22][0]),
                              action[23], true);
    eval_action_vertex_stra_8(&(iter[24][0]), &(iter[24][1]), &(iter[24][2]), &(iter[24][3]), &(iter[24][4]), &(iter[24][5]), &(iter[24][6]), &(iter[24][7]),
                              &(iter[23][0]), &(iter[23][1]), &(iter[23][2]), &(iter[23][3]), &(iter[23][4]), &(iter[23][5]), &(iter[23][6]), &(iter[23][7]), &(iter[23][8]),
                              action[24], false);
    eval_action_vertex_stra_8_split(&(iter[24][8]), &(iter[24][9]), &(iter[24][10]), &(iter[24][11]), &(iter[24][12]), &(iter[25][0]), &(iter[25][1]), &(iter[25][2]),
                                    &(iter[23][8]), &(iter[23][9]), &(iter[23][10]), &(iter[23][11]), &(iter[23][12]), &(iter[23][0]), &(iter[24][0]), &(iter[24][1]), &(iter[24][2]), &(iter[24][3]),
                                    action[24], action[25], FIVE, true);
    eval_action_vertex_stra_8(&(iter[25][3]), &(iter[25][4]), &(iter[25][5]), &(iter[25][6]), &(iter[25][7]), &(iter[25][8]), &(iter[25][9]), &(iter[25][10]),
                              &(iter[24][3]), &(iter[24][4]), &(iter[24][5]), &(iter[24][6]), &(iter[24][7]), &(iter[24][8]), &(iter[24][9]), &(iter[24][10]), &(iter[24][11]),
                              action[25], false);
    eval_action_vertex_stra_8_split(&(iter[25][11]), &(iter[25][12]), &(iter[26][0]), &(iter[26][1]), &(iter[26][2]), &(iter[26][3]), &(iter[26][4]), &(iter[26][5]),
                                    &(iter[24][11]), &(iter[24][12]), &(iter[24][0]), &(iter[25][0]), &(iter[25][1]), &(iter[25][2]), &(iter[25][3]), &(iter[25][4]), &(iter[25][5]), &(iter[25][6]),
                                    action[25], action[26], TWO, true);
    eval_action_vertex_stra_8_split(&(iter[26][6]), &(iter[26][7]), &(iter[26][8]), &(iter[26][9]), &(iter[26][10]), &(iter[26][11]), &(iter[26][12]), &(iter[27][0]),
                                    &(iter[25][6]), &(iter[25][7]), &(iter[25][8]), &(iter[25][9]), &(iter[25][10]), &(iter[25][11]), &(iter[25][12]), &(iter[25][0]), &(iter[26][0]), &(iter[26][1]),
                                    action[26], action[27], SEVEN, true);
    eval_action_vertex_stra_8(&(iter[27][1]), &(iter[27][2]), &(iter[27][3]), &(iter[27][4]), &(iter[27][5]), &(iter[27][6]), &(iter[27][7]), &(iter[27][8]),
                              &(iter[26][1]), &(iter[26][2]), &(iter[26][3]), &(iter[26][4]), &(iter[26][5]), &(iter[26][6]), &(iter[26][7]), &(iter[26][8]), &(iter[26][9]),
                              action[27], false);
    eval_action_vertex_stra_8_split(&(iter[27][9]), &(iter[27][10]), &(iter[27][11]), &(iter[27][12]), &(iter[28][0]), &(iter[28][1]), &(iter[28][2]), &(iter[28][3]),
                                    &(iter[26][9]), &(iter[26][10]), &(iter[26][11]), &(iter[26][12]), &(iter[26][0]), &(iter[27][0]), &(iter[27][1]), &(iter[27][2]), &(iter[27][3]), &(iter[27][4]),
                                    action[27], action[28], FOUR, true);
    eval_action_vertex_stra_8(&(iter[28][4]), &(iter[28][5]), &(iter[28][6]), &(iter[28][7]), &(iter[28][8]), &(iter[28][9]), &(iter[28][10]), &(iter[28][11]),
                              &(iter[27][4]), &(iter[27][5]), &(iter[27][6]), &(iter[27][7]), &(iter[27][8]), &(iter[27][9]), &(iter[27][10]), &(iter[27][11]), &(iter[27][12]),
                              action[28], false);
    eval_action_vertex_stra_8_split(&(iter[28][12]), &(iter[29][0]), &(iter[29][1]), &(iter[29][2]), &(iter[29][3]), &(iter[29][4]), &(iter[29][5]), &(iter[29][6]),
                                    &(iter[27][12]), &(iter[27][0]), &(iter[28][0]), &(iter[28][1]), &(iter[28][2]), &(iter[28][3]), &(iter[28][4]), &(iter[28][5]), &(iter[28][6]), &(iter[28][7]),
                                    action[28], action[29], ONE, true);
    eval_action_vertex_stra_8_split(&(iter[29][7]), &(iter[29][8]), &(iter[29][9]), &(iter[29][10]), &(iter[29][11]), &(iter[29][12]), &(iter[30][0]), &(iter[30][1]),
                                    &(iter[28][7]), &(iter[28][8]), &(iter[28][9]), &(iter[28][10]), &(iter[28][11]), &(iter[28][12]), &(iter[28][0]), &(iter[29][0]), &(iter[29][1]), &(iter[29][2]),
                                    action[29], action[30], SIX, true);
    eval_action_vertex_stra_8(&(iter[30][2]), &(iter[30][3]), &(iter[30][4]), &(iter[30][5]), &(iter[30][6]), &(iter[30][7]), &(iter[30][8]), &(iter[30][9]),
                              &(iter[29][2]), &(iter[29][3]), &(iter[29][4]), &(iter[29][5]), &(iter[29][6]), &(iter[29][7]), &(iter[29][8]), &(iter[29][9]), &(iter[29][10]),
                              action[30], false);
    eval_action_vertex_stra_8_split(&(iter[30][10]), &(iter[30][11]), &(iter[30][12]), &(iter[31][0]), &(iter[31][1]), &(iter[31][2]), &(iter[31][3]), &(iter[31][4]),
                                    &(iter[29][10]), &(iter[29][11]), &(iter[29][12]), &(iter[29][0]), &(iter[30][0]), &(iter[30][1]), &(iter[30][2]), &(iter[30][3]), &(iter[30][4]), &(iter[30][5]),
                                    action[30], action[31], THREE, true);
    eval_action_vertex_stra_8(&(iter[31][5]), &(iter[31][6]), &(iter[31][7]), &(iter[31][8]), &(iter[31][9]), &(iter[31][10]), &(iter[31][11]), &(iter[31][12]),
                              &(iter[30][5]), &(iter[30][6]), &(iter[30][7]), &(iter[30][8]), &(iter[30][9]), &(iter[30][10]), &(iter[30][11]), &(iter[30][12]), &(iter[30][0]),
                              action[31], true);
    eval_action_vertex_stra_8(&(iter[32][0]), &(iter[32][1]), &(iter[32][2]), &(iter[32][3]), &(iter[32][4]), &(iter[32][5]), &(iter[32][6]), &(iter[32][7]),
                              &(iter[31][0]), &(iter[31][1]), &(iter[31][2]), &(iter[31][3]), &(iter[31][4]), &(iter[31][5]), &(iter[31][6]), &(iter[31][7]), &(iter[31][8]),
                              action[32], false);
    eval_action_vertex_stra_8_split(&(iter[32][8]), &(iter[32][9]), &(iter[32][10]), &(iter[32][11]), &(iter[32][12]), &(iter[33][0]), &(iter[33][1]), &(iter[33][2]),
                                    &(iter[31][8]), &(iter[31][9]), &(iter[31][10]), &(iter[31][11]), &(iter[31][12]), &(iter[31][0]), &(iter[32][0]), &(iter[32][1]), &(iter[32][2]), &(iter[32][3]),
                                    action[32], action[33], FIVE, true);
    eval_action_vertex_stra_8(&(iter[33][3]), &(iter[33][4]), &(iter[33][5]), &(iter[33][6]), &(iter[33][7]), &(iter[33][8]), &(iter[33][9]), &(iter[33][10]),
                              &(iter[32][3]), &(iter[32][4]), &(iter[32][5]), &(iter[32][6]), &(iter[32][7]), &(iter[32][8]), &(iter[32][9]), &(iter[32][10]), &(iter[32][11]),
                              action[33], false);
    eval_action_vertex_stra_8_split(&(iter[33][11]), &(iter[33][12]), &((*out)[0]), &((*out)[1]), &((*out)[2]), &((*out)[3]), &((*out)[4]), &((*out)[5]),
                                    &(iter[32][11]), &(iter[32][12]), &(iter[32][0]), &(iter[33][0]), &(iter[33][1]), &(iter[33][2]), &(iter[33][3]), &(iter[33][4]), &(iter[33][5]), &(iter[33][6]),
                                    action[33], action[34], TWO, true);
    eval_action_vertex_stra_7(&((*out)[6]), &((*out)[7]), &((*out)[8]), &((*out)[9]), &((*out)[10]), &((*out)[11]), &((*out)[12]), 
                              &(iter[33][6]), &(iter[33][7]), &(iter[33][8]), &(iter[33][9]), &(iter[33][10]), &(iter[33][11]), &(iter[33][12]), &(iter[33][0]), 
                              action[34], true);
}

void final_action(fp2_t* out, const vertex_t* in1, const vertex_t* in2, secretkey_t s)
{
    vertex_t R[2];
    // copy_vertices(&(R[0]), &((*inp)[j]));
    // copy_vertices_swap_kernel(&(R[1]), &((*inp)[j + 1 == CYCLE_LENGTH ? 0 : j + 1]));
    copy_point(&(R[0].A24), &(in1->A24));
    copy_point(&(R[0].Ps), &(in1->Ps));
    copy_point(&(R[0].Pt), &(in1->Pt));
    copy_point(&(R[1].A24), &(in2->A24));
    copy_point(&(R[1].Ps), &(in2->Qs));
    copy_point(&(R[1].Pt), &(in2->Qt));
    int strascalar_i;
    ec_point_t B24;

    // Isogenies with kernel in E[p+1]
    strascalar_i = 0;
    ec_point_t stack_kernel[2][stack_volume];
    int stack_scalar[stack_volume];
    size_t top = 0;
    copy_point(&(stack_kernel[0][top]), &(R[0].Ps));
    copy_point(&(stack_kernel[1][top]), &(R[1].Ps));
    stack_scalar[top] = 0;
    top++;
    int jj = 0, k = 0;
    for(int i = 0; i < P_LEN; i++)
    {
        for(int t = 1; t <= TORSION_ODD_POWERS[i]; t++)
        {
            digit_t b = compare_gt(t, s[i]);
            jj++;
            for (int si = 0; si < top; si++)
            {
                swap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b);
            }
            swap_vertices(&(R[0]), &(R[1]), b);
            while(stack_scalar[top-1] < degree_s_num - jj)
            {
                stack_scalar[top] = stack_scalar[top-1] + strategy_s[k];
                // xMULv2(&(stack_kernel[0][top]), &(stack_kernel[0][top-1]), &(strascalar_s[strascalar_i]), strascalar_s_bits[k], &(R[0].A24));
                // if (stack_scalar[top] < degree_s_num - jj) // This conditional check does NOT leak any secret information
                // {
                //     xMULv2(&(stack_kernel[1][top]), &(stack_kernel[1][top-1]), &(strascalar_s[strascalar_i]), strascalar_s_bits[k], &(R[1].A24));
                // }
                // strascalar_i += strascalar_s_limbs[k];
                stra_scalar_func_s[strascalar_i](&(stack_kernel[0][top]), &(stack_kernel[0][top-1]), &(R[0].A24));
                if (stack_scalar[top] < degree_s_num - jj) // This conditional check does NOT leak any secret information
                {
                    stra_scalar_func_s[strascalar_i](&(stack_kernel[1][top]), &(stack_kernel[1][top-1]), &(R[1].A24));
                }
                strascalar_i++;
                top++;
                k++;
                assert(top <= stack_volume);
            }
            top--;
            assert(top >= 0);
            kps(i, stack_kernel[0][top], R[0].A24);
            xisog(&B24, i, R[0].A24);
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                xeval(&(stack_kernel[0][traverse_stack]), i, stack_kernel[0][traverse_stack], R[0].A24);
            }
            xeval(&(R[0].Pt), i, R[0].Pt, R[0].A24);
            xeval(&(R[0].Qs), i, R[0].Qs, R[0].A24);
            xeval(&(R[0].Qt), i, R[0].Qt, R[0].A24);
            copy_point(&(R[0].A24), &B24);
            kps_clear(i);
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                // xMULv2(&(stack_kernel[1][traverse_stack]), &(stack_kernel[1][traverse_stack]), &(TORSION_ODD_PRIMES[i]), p_plus_minus_bitlength[i], &(R[1].A24));
                torsionscalar_func[i](&(stack_kernel[1][traverse_stack]), &(stack_kernel[1][traverse_stack]), &(R[1].A24));
            }
            for (int si = 0; si < top; si++)
            {
                swap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b);
            }
            swap_vertices(&(R[0]), &(R[1]), b);
        }
    }
    assert(top == 0);

    // Isogenies with kernel in E[p-1]
    strascalar_i = 0;
    top = 0;
    copy_point(&(stack_kernel[0][top]), &(R[0].Pt));
    copy_point(&(stack_kernel[1][top]), &(R[1].Pt));
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
                swap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b);
            }
            swap_vertices(&(R[0]), &(R[1]), b);
            while(stack_scalar[top-1] < degree_t_num - jj)
            {
                stack_scalar[top] = stack_scalar[top-1] + strategy_t[k];
                // xMULv2(&(stack_kernel[0][top]), &(stack_kernel[0][top-1]), &(strascalar_t[strascalar_i]), strascalar_t_bits[k], &(R[0].A24));
                // if (stack_scalar[top] < degree_t_num - jj) // This conditional check does NOT leak any secret information
                // {
                //     xMULv2(&(stack_kernel[1][top]), &(stack_kernel[1][top-1]), &(strascalar_t[strascalar_i]), strascalar_t_bits[k], &(R[1].A24));
                // }
                // strascalar_i += strascalar_t_limbs[k];
                stra_scalar_func_t[strascalar_i](&(stack_kernel[0][top]), &(stack_kernel[0][top-1]), &(R[0].A24));
                if (stack_scalar[top] < degree_t_num - jj) // This conditional check does NOT leak any secret information
                {
                    stra_scalar_func_t[strascalar_i](&(stack_kernel[1][top]), &(stack_kernel[1][top-1]), &(R[1].A24));
                }
                strascalar_i++;
                top++;
                k++;
                assert(top <= stack_volume);
            }
            top--;
            assert(top >= 0);
            kps(i, stack_kernel[0][top], R[0].A24);
            xisog(&B24, i, R[0].A24);
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                xeval(&(stack_kernel[0][traverse_stack]), i, stack_kernel[0][traverse_stack], R[0].A24);
            }
            // xeval(&(R[b].Ps), i, R[b].Ps, R[b].A24);
            xeval(&(R[0].Qs), i, R[0].Qs, R[0].A24);
            xeval(&(R[0].Qt), i, R[0].Qt, R[0].A24);
            copy_point(&(R[0].A24), &B24);
            kps_clear(i);
            for(int traverse_stack = 0; traverse_stack < top; traverse_stack++)
            {
                // xMULv2(&(stack_kernel[1][traverse_stack]), &(stack_kernel[1][traverse_stack]), &(TORSION_ODD_PRIMES[i]), p_plus_minus_bitlength[i], &(R[1].A24));
                torsionscalar_func[i](&(stack_kernel[1][traverse_stack]), &(stack_kernel[1][traverse_stack]), &(R[1].A24));
            }
            for (int si = 0; si < top; si++)
            {
                swap_points(&(stack_kernel[0][si]), &(stack_kernel[1][si]), b);
            }
            swap_vertices(&(R[0]), &(R[1]), b);
        }
    }
    assert(top == 0);

    ec_curve_t E1;
    A24_to_AC(&E1, &(R[0].A24));
    ec_j_inv(out, &E1);
    #ifndef NDEBUG
        ec_curve_t E2;
        A24_to_AC(&E2, &(R[1].A24));
        fp2_t j2;
        ec_j_inv(&j2, &E2);
        assert(fp2_is_equal(out, &j2));
    #endif
}

void niike_SecretAgreement(fp2_t* out, const orient_cycle_t* in, secretkey_t s)
{
    secretkey_t action[ITERATIONS];
    orient_cycle_t iter[ITERATIONS - 1];
    memcpy(action[0], s, sizeof(secretkey_t));
    for(int i = 1; i < ITERATIONS; i++)
    {
        for(int j = 0; j < P_LEN + M_LEN; j++)
        {
            action[i][j] = action[i - 1][j] - TORSION_ODD_POWERS[j];
        }
    }
    eval_action_vertex_stra_8(&(iter[0][0]), &(iter[0][1]), &(iter[0][2]), &(iter[0][3]), &(iter[0][4]), &(iter[0][5]), &(iter[0][6]), &(iter[0][7]),
                              &((*in)[0]), &((*in)[1]), &((*in)[2]), &((*in)[3]), &((*in)[4]), &((*in)[5]), &((*in)[6]), &((*in)[7]), &((*in)[8]),
                              action[0], false);
    eval_action_vertex_stra_8_split(&(iter[0][8]), &(iter[0][9]), &(iter[0][10]), &(iter[0][11]), &(iter[0][12]), &(iter[1][0]), &(iter[1][1]), &(iter[1][2]),
                                    &((*in)[8]), &((*in)[9]), &((*in)[10]), &((*in)[11]), &((*in)[12]), &((*in)[0]), &(iter[0][0]), &(iter[0][1]), &(iter[0][2]), &(iter[0][3]),
                                    action[0], action[1], FIVE, true);
    eval_action_vertex_stra_8(&(iter[1][3]), &(iter[1][4]), &(iter[1][5]), &(iter[1][6]), &(iter[1][7]), &(iter[1][8]), &(iter[1][9]), &(iter[1][10]),
                              &(iter[0][3]), &(iter[0][4]), &(iter[0][5]), &(iter[0][6]), &(iter[0][7]), &(iter[0][8]), &(iter[0][9]), &(iter[0][10]), &(iter[0][11]),
                              action[1], false);
    eval_action_vertex_stra_8_split(&(iter[1][11]), &(iter[1][12]), &(iter[2][0]), &(iter[2][1]), &(iter[2][2]), &(iter[2][3]), &(iter[2][4]), &(iter[2][5]),
                                    &(iter[0][11]), &(iter[0][12]), &(iter[0][0]), &(iter[1][0]), &(iter[1][1]), &(iter[1][2]), &(iter[1][3]), &(iter[1][4]), &(iter[1][5]), &(iter[1][6]),
                                    action[1], action[2], TWO, true);
    eval_action_vertex_stra_8_split(&(iter[2][6]), &(iter[2][7]), &(iter[2][8]), &(iter[2][9]), &(iter[2][10]), &(iter[2][11]), &(iter[2][12]), &(iter[3][0]),
                                    &(iter[1][6]), &(iter[1][7]), &(iter[1][8]), &(iter[1][9]), &(iter[1][10]), &(iter[1][11]), &(iter[1][12]), &(iter[1][0]), &(iter[2][0]), &(iter[2][1]),
                                    action[2], action[3], SEVEN, true);
    eval_action_vertex_stra_8(&(iter[3][1]), &(iter[3][2]), &(iter[3][3]), &(iter[3][4]), &(iter[3][5]), &(iter[3][6]), &(iter[3][7]), &(iter[3][8]),
                              &(iter[2][1]), &(iter[2][2]), &(iter[2][3]), &(iter[2][4]), &(iter[2][5]), &(iter[2][6]), &(iter[2][7]), &(iter[2][8]), &(iter[2][9]),
                              action[3], false);
    eval_action_vertex_stra_8_split(&(iter[3][9]), &(iter[3][10]), &(iter[3][11]), &(iter[3][12]), &(iter[4][0]), &(iter[4][1]), &(iter[4][2]), &(iter[4][3]),
                                    &(iter[2][9]), &(iter[2][10]), &(iter[2][11]), &(iter[2][12]), &(iter[2][0]), &(iter[3][0]), &(iter[3][1]), &(iter[3][2]), &(iter[3][3]), &(iter[3][4]),
                                    action[3], action[4], FOUR, true);
    eval_action_vertex_stra_8(&(iter[4][4]), &(iter[4][5]), &(iter[4][6]), &(iter[4][7]), &(iter[4][8]), &(iter[4][9]), &(iter[4][10]), &(iter[4][11]),
                              &(iter[3][4]), &(iter[3][5]), &(iter[3][6]), &(iter[3][7]), &(iter[3][8]), &(iter[3][9]), &(iter[3][10]), &(iter[3][11]), &(iter[3][12]),
                              action[4], false);
    eval_action_vertex_stra_8_split(&(iter[4][12]), &(iter[5][0]), &(iter[5][1]), &(iter[5][2]), &(iter[5][3]), &(iter[5][4]), &(iter[5][5]), &(iter[5][6]),
                                    &(iter[3][12]), &(iter[3][0]), &(iter[4][0]), &(iter[4][1]), &(iter[4][2]), &(iter[4][3]), &(iter[4][4]), &(iter[4][5]), &(iter[4][6]), &(iter[4][7]),
                                    action[4], action[5], ONE, true);
    eval_action_vertex_stra_8_split(&(iter[5][7]), &(iter[5][8]), &(iter[5][9]), &(iter[5][10]), &(iter[5][11]), &(iter[5][12]), &(iter[6][0]), &(iter[6][1]),
                                    &(iter[4][7]), &(iter[4][8]), &(iter[4][9]), &(iter[4][10]), &(iter[4][11]), &(iter[4][12]), &(iter[4][0]), &(iter[5][0]), &(iter[5][1]), &(iter[5][2]),
                                    action[5], action[6], SIX, true);
    eval_action_vertex_stra_8(&(iter[6][2]), &(iter[6][3]), &(iter[6][4]), &(iter[6][5]), &(iter[6][6]), &(iter[6][7]), &(iter[6][8]), &(iter[6][9]),
                              &(iter[5][2]), &(iter[5][3]), &(iter[5][4]), &(iter[5][5]), &(iter[5][6]), &(iter[5][7]), &(iter[5][8]), &(iter[5][9]), &(iter[5][10]),
                              action[6], false);
    eval_action_vertex_stra_8_split(&(iter[6][10]), &(iter[6][11]), &(iter[6][12]), &(iter[7][0]), &(iter[7][1]), &(iter[7][2]), &(iter[7][3]), &(iter[7][4]),
                                    &(iter[5][10]), &(iter[5][11]), &(iter[5][12]), &(iter[5][0]), &(iter[6][0]), &(iter[6][1]), &(iter[6][2]), &(iter[6][3]), &(iter[6][4]), &(iter[6][5]),
                                    action[6], action[7], THREE, true);
    eval_action_vertex_stra_8(&(iter[7][5]), &(iter[7][6]), &(iter[7][7]), &(iter[7][8]), &(iter[7][9]), &(iter[7][10]), &(iter[7][11]), &(iter[7][12]),
                              &(iter[6][5]), &(iter[6][6]), &(iter[6][7]), &(iter[6][8]), &(iter[6][9]), &(iter[6][10]), &(iter[6][11]), &(iter[6][12]), &(iter[6][0]),
                              action[7], true);
    eval_action_vertex_stra_8(&(iter[8][0]), &(iter[8][1]), &(iter[8][2]), &(iter[8][3]), &(iter[8][4]), &(iter[8][5]), &(iter[8][6]), &(iter[8][7]),
                              &(iter[7][0]), &(iter[7][1]), &(iter[7][2]), &(iter[7][3]), &(iter[7][4]), &(iter[7][5]), &(iter[7][6]), &(iter[7][7]), &(iter[7][8]),
                              action[8], false);
    eval_action_vertex_stra_8_split(&(iter[8][8]), &(iter[8][9]), &(iter[8][10]), &(iter[8][11]), &(iter[8][12]), &(iter[9][0]), &(iter[9][1]), &(iter[9][2]),
                                    &(iter[7][8]), &(iter[7][9]), &(iter[7][10]), &(iter[7][11]), &(iter[7][12]), &(iter[7][0]), &(iter[8][0]), &(iter[8][1]), &(iter[8][2]), &(iter[8][3]),
                                    action[8], action[9], FIVE, true);
    eval_action_vertex_stra_8(&(iter[9][3]), &(iter[9][4]), &(iter[9][5]), &(iter[9][6]), &(iter[9][7]), &(iter[9][8]), &(iter[9][9]), &(iter[9][10]),
                              &(iter[8][3]), &(iter[8][4]), &(iter[8][5]), &(iter[8][6]), &(iter[8][7]), &(iter[8][8]), &(iter[8][9]), &(iter[8][10]), &(iter[8][11]),
                              action[9], false);
    eval_action_vertex_stra_8_split(&(iter[9][11]), &(iter[9][12]), &(iter[10][0]), &(iter[10][1]), &(iter[10][2]), &(iter[10][3]), &(iter[10][4]), &(iter[10][5]),
                                    &(iter[8][11]), &(iter[8][12]), &(iter[8][0]), &(iter[9][0]), &(iter[9][1]), &(iter[9][2]), &(iter[9][3]), &(iter[9][4]), &(iter[9][5]), &(iter[9][6]),
                                    action[9], action[10], TWO, true);
    eval_action_vertex_stra_8_split(&(iter[10][6]), &(iter[10][7]), &(iter[10][8]), &(iter[10][9]), &(iter[10][10]), &(iter[10][11]), &(iter[10][12]), &(iter[11][0]),
                                    &(iter[9][6]), &(iter[9][7]), &(iter[9][8]), &(iter[9][9]), &(iter[9][10]), &(iter[9][11]), &(iter[9][12]), &(iter[9][0]), &(iter[10][0]), &(iter[10][1]),
                                    action[10], action[11], SEVEN, true);
    eval_action_vertex_stra_8(&(iter[11][1]), &(iter[11][2]), &(iter[11][3]), &(iter[11][4]), &(iter[11][5]), &(iter[11][6]), &(iter[11][7]), &(iter[11][8]),
                              &(iter[10][1]), &(iter[10][2]), &(iter[10][3]), &(iter[10][4]), &(iter[10][5]), &(iter[10][6]), &(iter[10][7]), &(iter[10][8]), &(iter[10][9]),
                              action[11], false);
    eval_action_vertex_stra_8_split(&(iter[11][9]), &(iter[11][10]), &(iter[11][11]), &(iter[11][12]), &(iter[12][0]), &(iter[12][1]), &(iter[12][2]), &(iter[12][3]),
                                    &(iter[10][9]), &(iter[10][10]), &(iter[10][11]), &(iter[10][12]), &(iter[10][0]), &(iter[11][0]), &(iter[11][1]), &(iter[11][2]), &(iter[11][3]), &(iter[11][4]),
                                    action[11], action[12], FOUR, true);
    eval_action_vertex_stra_8(&(iter[12][4]), &(iter[12][5]), &(iter[12][6]), &(iter[12][7]), &(iter[12][8]), &(iter[12][9]), &(iter[12][10]), &(iter[12][11]),
                              &(iter[11][4]), &(iter[11][5]), &(iter[11][6]), &(iter[11][7]), &(iter[11][8]), &(iter[11][9]), &(iter[11][10]), &(iter[11][11]), &(iter[11][12]),
                              action[12], false);
    eval_action_vertex_stra_8_split(&(iter[12][12]), &(iter[13][0]), &(iter[13][1]), &(iter[13][2]), &(iter[13][3]), &(iter[13][4]), &(iter[13][5]), &(iter[13][6]),
                                    &(iter[11][12]), &(iter[11][0]), &(iter[12][0]), &(iter[12][1]), &(iter[12][2]), &(iter[12][3]), &(iter[12][4]), &(iter[12][5]), &(iter[12][6]), &(iter[12][7]),
                                    action[12], action[13], ONE, true);
    eval_action_vertex_stra_8_split(&(iter[13][7]), &(iter[13][8]), &(iter[13][9]), &(iter[13][10]), &(iter[13][11]), &(iter[13][12]), &(iter[14][0]), &(iter[14][1]),
                                    &(iter[12][7]), &(iter[12][8]), &(iter[12][9]), &(iter[12][10]), &(iter[12][11]), &(iter[12][12]), &(iter[12][0]), &(iter[13][0]), &(iter[13][1]), &(iter[13][2]),
                                    action[13], action[14], SIX, true);
    eval_action_vertex_stra_8(&(iter[14][2]), &(iter[14][3]), &(iter[14][4]), &(iter[14][5]), &(iter[14][6]), &(iter[14][7]), &(iter[14][8]), &(iter[14][9]),
                              &(iter[13][2]), &(iter[13][3]), &(iter[13][4]), &(iter[13][5]), &(iter[13][6]), &(iter[13][7]), &(iter[13][8]), &(iter[13][9]), &(iter[13][10]),
                              action[14], false);
    eval_action_vertex_stra_8_split(&(iter[14][10]), &(iter[14][11]), &(iter[14][12]), &(iter[15][0]), &(iter[15][1]), &(iter[15][2]), &(iter[15][3]), &(iter[15][4]),
                                    &(iter[13][10]), &(iter[13][11]), &(iter[13][12]), &(iter[13][0]), &(iter[14][0]), &(iter[14][1]), &(iter[14][2]), &(iter[14][3]), &(iter[14][4]), &(iter[14][5]),
                                    action[14], action[15], THREE, true);
    eval_action_vertex_stra_8(&(iter[15][5]), &(iter[15][6]), &(iter[15][7]), &(iter[15][8]), &(iter[15][9]), &(iter[15][10]), &(iter[15][11]), &(iter[15][12]),
                              &(iter[14][5]), &(iter[14][6]), &(iter[14][7]), &(iter[14][8]), &(iter[14][9]), &(iter[14][10]), &(iter[14][11]), &(iter[14][12]), &(iter[14][0]),
                              action[15], true);
    eval_action_vertex_stra_8(&(iter[16][0]), &(iter[16][1]), &(iter[16][2]), &(iter[16][3]), &(iter[16][4]), &(iter[16][5]), &(iter[16][6]), &(iter[16][7]),
                              &(iter[15][0]), &(iter[15][1]), &(iter[15][2]), &(iter[15][3]), &(iter[15][4]), &(iter[15][5]), &(iter[15][6]), &(iter[15][7]), &(iter[15][8]),
                              action[16], false);
    eval_action_vertex_stra_8_split(&(iter[16][8]), &(iter[16][9]), &(iter[16][10]), &(iter[16][11]), &(iter[16][12]), &(iter[17][0]), &(iter[17][1]), &(iter[17][2]),
                                    &(iter[15][8]), &(iter[15][9]), &(iter[15][10]), &(iter[15][11]), &(iter[15][12]), &(iter[15][0]), &(iter[16][0]), &(iter[16][1]), &(iter[16][2]), &(iter[16][3]),
                                    action[16], action[17], FIVE, true);
    eval_action_vertex_stra_8(&(iter[17][3]), &(iter[17][4]), &(iter[17][5]), &(iter[17][6]), &(iter[17][7]), &(iter[17][8]), &(iter[17][9]), &(iter[17][10]),
                              &(iter[16][3]), &(iter[16][4]), &(iter[16][5]), &(iter[16][6]), &(iter[16][7]), &(iter[16][8]), &(iter[16][9]), &(iter[16][10]), &(iter[16][11]),
                              action[17], false);
    eval_action_vertex_stra_8_split(&(iter[17][11]), &(iter[17][12]), &(iter[18][0]), &(iter[18][1]), &(iter[18][2]), &(iter[18][3]), &(iter[18][4]), &(iter[18][5]),
                                    &(iter[16][11]), &(iter[16][12]), &(iter[16][0]), &(iter[17][0]), &(iter[17][1]), &(iter[17][2]), &(iter[17][3]), &(iter[17][4]), &(iter[17][5]), &(iter[17][6]),
                                    action[17], action[18], TWO, true);
    eval_action_vertex_stra_8_split(&(iter[18][6]), &(iter[18][7]), &(iter[18][8]), &(iter[18][9]), &(iter[18][10]), &(iter[18][11]), &(iter[18][12]), &(iter[19][0]),
                                    &(iter[17][6]), &(iter[17][7]), &(iter[17][8]), &(iter[17][9]), &(iter[17][10]), &(iter[17][11]), &(iter[17][12]), &(iter[17][0]), &(iter[18][0]), &(iter[18][1]),
                                    action[18], action[19], SEVEN, true);
    eval_action_vertex_stra_8(&(iter[19][1]), &(iter[19][2]), &(iter[19][3]), &(iter[19][4]), &(iter[19][5]), &(iter[19][6]), &(iter[19][7]), &(iter[19][8]),
                              &(iter[18][1]), &(iter[18][2]), &(iter[18][3]), &(iter[18][4]), &(iter[18][5]), &(iter[18][6]), &(iter[18][7]), &(iter[18][8]), &(iter[18][9]),
                              action[19], false);
    eval_action_vertex_stra_8_split(&(iter[19][9]), &(iter[19][10]), &(iter[19][11]), &(iter[19][12]), &(iter[20][0]), &(iter[20][1]), &(iter[20][2]), &(iter[20][3]),
                                    &(iter[18][9]), &(iter[18][10]), &(iter[18][11]), &(iter[18][12]), &(iter[18][0]), &(iter[19][0]), &(iter[19][1]), &(iter[19][2]), &(iter[19][3]), &(iter[19][4]),
                                    action[19], action[20], FOUR, true);
    eval_action_vertex_stra_8(&(iter[20][4]), &(iter[20][5]), &(iter[20][6]), &(iter[20][7]), &(iter[20][8]), &(iter[20][9]), &(iter[20][10]), &(iter[20][11]),
                              &(iter[19][4]), &(iter[19][5]), &(iter[19][6]), &(iter[19][7]), &(iter[19][8]), &(iter[19][9]), &(iter[19][10]), &(iter[19][11]), &(iter[19][12]),
                              action[20], false);
    eval_action_vertex_stra_8_split(&(iter[20][12]), &(iter[21][0]), &(iter[21][1]), &(iter[21][2]), &(iter[21][3]), &(iter[21][4]), &(iter[21][5]), &(iter[21][6]),
                                    &(iter[19][12]), &(iter[19][0]), &(iter[20][0]), &(iter[20][1]), &(iter[20][2]), &(iter[20][3]), &(iter[20][4]), &(iter[20][5]), &(iter[20][6]), &(iter[20][7]),
                                    action[20], action[21], ONE, true);
    eval_action_vertex_stra_8_split(&(iter[21][7]), &(iter[21][8]), &(iter[21][9]), &(iter[21][10]), &(iter[21][11]), &(iter[21][12]), &(iter[22][0]), &(iter[22][1]),
                                    &(iter[20][7]), &(iter[20][8]), &(iter[20][9]), &(iter[20][10]), &(iter[20][11]), &(iter[20][12]), &(iter[20][0]), &(iter[21][0]), &(iter[21][1]), &(iter[21][2]),
                                    action[21], action[22], SIX, true);
    eval_action_vertex_stra_8(&(iter[22][2]), &(iter[22][3]), &(iter[22][4]), &(iter[22][5]), &(iter[22][6]), &(iter[22][7]), &(iter[22][8]), &(iter[22][9]),
                              &(iter[21][2]), &(iter[21][3]), &(iter[21][4]), &(iter[21][5]), &(iter[21][6]), &(iter[21][7]), &(iter[21][8]), &(iter[21][9]), &(iter[21][10]),
                              action[22], false);
    eval_action_vertex_stra_8_split(&(iter[22][10]), &(iter[22][11]), &(iter[22][12]), &(iter[23][0]), &(iter[23][1]), &(iter[23][2]), &(iter[23][3]), &(iter[23][4]),
                                    &(iter[21][10]), &(iter[21][11]), &(iter[21][12]), &(iter[21][0]), &(iter[22][0]), &(iter[22][1]), &(iter[22][2]), &(iter[22][3]), &(iter[22][4]), &(iter[22][5]),
                                    action[22], action[23], THREE, true);
    eval_action_vertex_stra_8_split(&(iter[23][5]), &(iter[23][6]), &(iter[23][7]), &(iter[23][8]), &(iter[23][9]), &(iter[23][10]), &(iter[23][11]), &(iter[24][0]),
                                    &(iter[22][5]), &(iter[22][6]), &(iter[22][7]), &(iter[22][8]), &(iter[22][9]), &(iter[22][10]), &(iter[22][11]), &(iter[22][12]), &(iter[23][0]), &(iter[23][1]),
                                    action[23], action[24], SEVEN, false);
    eval_action_vertex_stra_8(&(iter[24][1]), &(iter[24][2]), &(iter[24][3]), &(iter[24][4]), &(iter[24][5]), &(iter[24][6]), &(iter[24][7]), &(iter[24][8]),
                              &(iter[23][1]), &(iter[23][2]), &(iter[23][3]), &(iter[23][4]), &(iter[23][5]), &(iter[23][6]), &(iter[23][7]), &(iter[23][8]), &(iter[23][9]),
                              action[24], false);
    eval_action_vertex_stra_8_split(&(iter[24][9]), &(iter[24][10]), &(iter[25][0]), &(iter[25][1]), &(iter[25][2]), &(iter[25][3]), &(iter[25][4]), &(iter[25][5]),
                                    &(iter[23][9]), &(iter[23][10]), &(iter[23][11]), &(iter[24][0]), &(iter[24][1]), &(iter[24][2]), &(iter[24][3]), &(iter[24][4]), &(iter[24][5]), &(iter[24][6]),
                                    action[24], action[25], TWO, false);
    eval_action_vertex_stra_8_split(&(iter[25][6]), &(iter[25][7]), &(iter[25][8]), &(iter[25][9]), &(iter[26][0]), &(iter[26][1]), &(iter[26][2]), &(iter[26][3]),
                                    &(iter[24][6]), &(iter[24][7]), &(iter[24][8]), &(iter[24][9]), &(iter[24][10]), &(iter[25][0]), &(iter[25][1]), &(iter[25][2]), &(iter[25][3]), &(iter[25][4]),
                                    action[25], action[26], FOUR, false);
    eval_action_vertex_stra_5(&(iter[26][4]), &(iter[26][5]), &(iter[26][6]), &(iter[26][7]), &(iter[26][8]),
                              &(iter[25][4]), &(iter[25][5]), &(iter[25][6]), &(iter[25][7]), &(iter[25][8]), &(iter[25][9]),
                              action[26], false);
    eval_action_vertex_stra_8(&(iter[27][0]), &(iter[27][1]), &(iter[27][2]), &(iter[27][3]), &(iter[27][4]), &(iter[27][5]), &(iter[27][6]), &(iter[27][7]),
                              &(iter[26][0]), &(iter[26][1]), &(iter[26][2]), &(iter[26][3]), &(iter[26][4]), &(iter[26][5]), &(iter[26][6]), &(iter[26][7]), &(iter[26][8]),
                              action[27], false);
    eval_action_vertex_stra_7(&(iter[28][0]), &(iter[28][1]), &(iter[28][2]), &(iter[28][3]), &(iter[28][4]), &(iter[28][5]), &(iter[28][6]),
                              &(iter[27][0]), &(iter[27][1]), &(iter[27][2]), &(iter[27][3]), &(iter[27][4]), &(iter[27][5]), &(iter[27][6]), &(iter[27][7]),
                              action[28], false);
    eval_action_vertex_stra_6(&(iter[29][0]), &(iter[29][1]), &(iter[29][2]), &(iter[29][3]), &(iter[29][4]), &(iter[29][5]),
                              &(iter[28][0]), &(iter[28][1]), &(iter[28][2]), &(iter[28][3]), &(iter[28][4]), &(iter[28][5]), &(iter[28][6]),
                              action[29], false);
    eval_action_vertex_stra_5(&(iter[30][0]), &(iter[30][1]), &(iter[30][2]), &(iter[30][3]), &(iter[30][4]),
                              &(iter[29][0]), &(iter[29][1]), &(iter[29][2]), &(iter[29][3]), &(iter[29][4]), &(iter[29][5]),
                              action[30], false);
    eval_action_vertex_stra_4(&(iter[31][0]), &(iter[31][1]), &(iter[31][2]), &(iter[31][3]),
                              &(iter[30][0]), &(iter[30][1]), &(iter[30][2]), &(iter[30][3]), &(iter[30][4]),
                              action[31], false);
    eval_action_vertex_stra_3(&(iter[32][0]), &(iter[32][1]), &(iter[32][2]),
                              &(iter[31][0]), &(iter[31][1]), &(iter[31][2]), &(iter[31][3]),
                              action[32], false);
    eval_action_vertex_stra_2(&(iter[33][0]), &(iter[33][1]),
                              &(iter[32][0]), &(iter[32][1]), &(iter[32][2]),
                              action[33], false);
    final_action(out, &(iter[33][0]), &(iter[33][1]), action[34]);
}

void encode_orient_cycle(void *dst, const orient_cycle_t *in)
{
    fp2_t invlist[5*CYCLE_LENGTH];
    for (int i = 0; i < CYCLE_LENGTH; i++)
    {
        fp2_copy(&(invlist[5*i]), &((*in)[i].A24.z));
        fp2_copy(&(invlist[5*i + 1]), &((*in)[i].Ps.z));
        fp2_copy(&(invlist[5*i + 2]), &((*in)[i].Pt.z));
        fp2_copy(&(invlist[5*i + 3]), &((*in)[i].Qs.z));
        fp2_copy(&(invlist[5*i + 4]), &((*in)[i].Qt.z));
    }
    fp2_batched_inv(invlist, 5*CYCLE_LENGTH);
    fp2_t result;
    for (int i = 0; i < CYCLE_LENGTH; i++)
    {
        fp2_mul(&result, &((*in)[i].A24.x), &(invlist[5*i]));
        fp2_encode(dst+ 5 * i * FP2_ENCODED_BYTES, &result);
        fp2_mul(&result, &((*in)[i].Ps.x), &(invlist[5*i + 1]));
        fp2_encode(dst+ (5*i + 1) * FP2_ENCODED_BYTES, &result);
        fp2_mul(&result, &((*in)[i].Pt.x), &(invlist[5*i + 2]));
        fp2_encode(dst+ (5*i + 2) * FP2_ENCODED_BYTES, &result);
        fp2_mul(&result, &((*in)[i].Qs.x), &(invlist[5*i + 3]));
        fp2_encode(dst+ (5*i + 3) * FP2_ENCODED_BYTES, &result);
        fp2_mul(&result, &((*in)[i].Qt.x), &(invlist[5*i + 4]));
        fp2_encode(dst+ (5*i + 4) * FP2_ENCODED_BYTES, &result);
    }
}

void decode_orient_cycle(orient_cycle_t *out, void *in)
{
    for (int i = 0; i < CYCLE_LENGTH; i++)
    {
        fp2_decode(&((*out)[i].A24.x), in + 5 * i * FP2_ENCODED_BYTES);
        fp2_setone(&((*out)[i].A24.z));
        fp2_decode(&((*out)[i].Ps.x), in + (5*i + 1) * FP2_ENCODED_BYTES);
        fp2_setone(&((*out)[i].Ps.z));
        fp2_decode(&((*out)[i].Pt.x), in + (5*i + 2) * FP2_ENCODED_BYTES);
        fp2_setone(&((*out)[i].Pt.z));
        fp2_decode(&((*out)[i].Qs.x), in + (5*i + 3) * FP2_ENCODED_BYTES);
        fp2_setone(&((*out)[i].Qs.z));
        fp2_decode(&((*out)[i].Qt.x), in + (5*i + 4) * FP2_ENCODED_BYTES);
        fp2_setone(&((*out)[i].Qt.z));
    }
}