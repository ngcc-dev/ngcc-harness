#include <quaternion.h>
#define MAXORD_O0 (EXTREMAL_ORDERS->order)
#define STANDARD_EXTREMAL_ORDER (EXTREMAL_ORDERS[0])
#define NUM_ALTERNATE_EXTREMAL_ORDERS 0
#define ALTERNATE_EXTREMAL_ORDERS (EXTREMAL_ORDERS+1)
#define ALTERNATE_CONNECTING_IDEALS (CONNECTING_IDEALS+1)
#define ALTERNATE_CONJUGATING_ELEMENTS (CONJUGATING_ELEMENTS+1)
extern const short QUAT_small_primes_where_neg_q_square[101];
extern const ibz_t QUAT_prods_of_bad_primes[1];
extern const ibz_t QUAT_prime_cofactor;
extern const quat_alg_t QUATALG_PINFTY;
extern const quat_p_extremal_maximal_order_t EXTREMAL_ORDERS[1];
extern const quat_left_ideal_t CONNECTING_IDEALS[1];
extern const quat_alg_elem_t CONJUGATING_ELEMENTS[1];
