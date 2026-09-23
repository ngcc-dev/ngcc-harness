// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from torsion_constants.c in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modified to adapt to NIIKE
 */
                        
#include <stddef.h>
#include <stdint.h>
#include <torsion_constants.h>

const uint64_t TORSION_ODD_PRIMES[84] = {2, 3, 7, 11, 13, 17, 19, 23, 31, 37, 47, 53, 59, 73, 83, 97, 101, 103, 107, 113, 127, 131, 137, 139, 149, 151, 163, 167, 179, 181, 191, 223, 227, 229, 233, 239, 257, 263, 269, 281, 307, 311, 313, 317, 337, 347, 367, 373, 379, 383, 461, 593, 787, 937, 5, 29, 41, 43, 61, 67, 71, 79, 89, 109, 157, 173, 193, 197, 199, 211, 241, 251, 271, 277, 283, 293, 331, 349, 353, 359, 443, 587, 601, 607};
const uint64_t TORSION_ODD_POWERS[84] = {6, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
