#ifndef LOADER_H
#define LOADER_H

#include "ngcc_api.h"

typedef struct {
    void *handle;
    ngcc_api_t api;
} ngcc_library_t;

/**
 * Load a shared library and resolve symbols for the selected algorithm groups.
 *
 * @param lib_path  Path to the .so file.
 * @param test_mask Bitmask of TEST_MASK_HASH/SIG/KEM/KEX selecting which
 *                  symbol groups to load.  Unselected groups' function pointers
 *                  remain NULL.
 * @param out_lib   Receives the opened library handle and resolved API.
 * @return 0 on success, -1 on error.
 */
int ngcc_load_library(const char *lib_path, unsigned int test_mask,
                      ngcc_library_t *out_lib);
void ngcc_unload_library(ngcc_library_t *lib);

#endif
