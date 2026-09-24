#ifndef CRETAKE_PRIMITIVE_INTERFACES_H
#define CRETAKE_PRIMITIVE_INTERFACES_H

/*
 * Import the selected KEM/PKE primitive while isolating generic
 * submission macros and the generic PARAMS_H include guard.
 */
#pragma push_macro("OUTPUT_BLANK_TEST_VECTORS")
#pragma push_macro("ALGORITHM_INSTANCE")
#pragma push_macro("PARAMS_H")

#undef OUTPUT_BLANK_TEST_VECTORS
#undef ALGORITHM_INSTANCE
#undef PARAMS_H

#include "../../Common/primitives/kem/ZEN_512/params.h"
#include "../../Common/primitives/kem/ZEN_512/KEM_AlgorithmInstance.h"
#include "../../Common/primitives/kem/ZEN_512/pke.h"

#undef OUTPUT_BLANK_TEST_VECTORS
#undef ALGORITHM_INSTANCE
#undef PARAMS_H

#pragma pop_macro("PARAMS_H")
#pragma pop_macro("ALGORITHM_INSTANCE")
#pragma pop_macro("OUTPUT_BLANK_TEST_VECTORS")


/*
 * Import the selected signature primitive while isolating generic
 * submission macros and the generic PARAMS_H include guard.
 */
#pragma push_macro("OUTPUT_BLANK_TEST_VECTORS")
#pragma push_macro("ALGORITHM_INSTANCE")
#pragma push_macro("PARAMS_H")

#undef OUTPUT_BLANK_TEST_VECTORS
#undef ALGORITHM_INSTANCE
#undef PARAMS_H

#include "../../Common/primitives/sig/BiT-512/params.h"
#include "../../Common/primitives/sig/BiT-512/SIG_AlgorithmInstance.h"

#undef OUTPUT_BLANK_TEST_VECTORS
#undef ALGORITHM_INSTANCE
#undef PARAMS_H

#pragma pop_macro("PARAMS_H")
#pragma pop_macro("ALGORITHM_INSTANCE")
#pragma pop_macro("OUTPUT_BLANK_TEST_VECTORS")

#endif
