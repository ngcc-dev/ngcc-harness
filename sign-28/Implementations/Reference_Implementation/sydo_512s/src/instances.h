/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_REF_INSTANCES_H
#define SYDO_REF_INSTANCES_H

#include "macros.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_LAMBDA 512
#define MAX_LAMBDA_BYTES (MAX_LAMBDA / 8)
#define MAX_DEPTH 12
#define MAX_TAU 64
#define UNIVERSAL_HASH_B_BITS 16
#define UNIVERSAL_HASH_B (UNIVERSAL_HASH_B_BITS / 8)
#define AES_BLOCK_SIZE 16
#define IV_SIZE AES_BLOCK_SIZE

#endif
