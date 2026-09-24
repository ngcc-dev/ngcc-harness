/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_DEFINES_H
#define SYDO_DEFINES_H

#define SYDO_EXPORT
#define SYDO_CALLING_CONVENTION

#ifdef __cplusplus
#define SYDO_BEGIN_C_DECL extern "C" {
#define SYDO_END_C_DECL }
#else
#define SYDO_BEGIN_C_DECL
#define SYDO_END_C_DECL
#endif

#endif
