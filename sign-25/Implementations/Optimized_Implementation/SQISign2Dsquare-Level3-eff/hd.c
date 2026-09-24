#include <hd.h>
#include <assert.h>



/**
 * @brief Compute the double of the theta couple jac point in on the elliptic product E12
 *
 * @param out Output: the theta_couple_jac_point
 * @param E12 an elliptic product
 * @param in the theta couple jac point in the elliptic product
 * in = (P1,P2)
 * out = [2] (P1,P2)
 *
 */
void
double_couple_jac_point(theta_couple_jac_point_t *out,
                        const theta_couple_curve_t *A,
                        const theta_couple_jac_point_t *in)
{
    DBL(&out->P1, &in->P1, &A->E1);
    DBL(&out->P2, &in->P2, &A->E2);
}

/**
 * @brief Compute the iterated double of the theta couple jac point in on the elliptic product E12
 *
 * @param out Output: the theta_couple_jac_point
 * @param n : the number of iteration
 * @param E12 an elliptic product
 * @param in the theta couple jac point in the elliptic product
 * in = (P1,P2)
 * out = [2^n] (P1,P2)
 *
 */
void
double_couple_jac_point_iter(theta_couple_jac_point_t *out,
                             int n,
                             const theta_couple_curve_t *A,
                             const theta_couple_jac_point_t *in)
{
    if (n == 0) {
        *out = *in;
    } else {
        double_couple_jac_point(out, A, in);
        for (int i = 0; i < n - 1; i++) {
            double_couple_jac_point(out, A, out);
        }
    }
}

void
couple_jac_to_xz(theta_couple_point_t *P, const theta_couple_jac_point_t *xyP)
{
    jac_to_xz(&P->P1, &xyP->P1);
    jac_to_xz(&P->P2, &xyP->P2);
}

