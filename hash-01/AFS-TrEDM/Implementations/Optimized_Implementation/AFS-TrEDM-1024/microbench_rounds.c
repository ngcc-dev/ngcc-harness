/*
 * Instance-local wrapper for the Stage S6-02B round microbenchmark.
 * Keep afs_p1600.c in this translation unit only so static round helpers are
 * visible without leaking into other linked sources.
 */
#include "afs_p1600.c"
#include "../common/s6/microbench_rounds.c"
