#ifndef XOF_NGCC_HPP
#define XOF_NGCC_HPP

/*
 * xof_ngcc.hpp -- XOF/Hash adapter from NGCC pseudoXOF to the FAEST C++ hash API.
 *
 * Replaces hash.hpp's Keccak/SHAKE backend with the NGCC auxiliary pseudoXOF
 * (SM3-based). This is a PLACEHOLDER per decision D5: not perf-tuned, just
 * correct. All optimizations other than the XOF are retained.
 *
 * The FAEST C++ code uses hash_state / hash_state_x4 with init/update/finalize.
 * We provide a stateful wrapper that buffers absorbed data and calls pseudoXOF
 * on finalize/squeeze.
 */

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include "../reference/ngcc/auxfunc.h"

namespace faest {

struct xof_ngcc_state {
    std::vector<uint8_t> buf;
    bool finalized = false;
    std::vector<uint8_t> out;
    size_t out_used = 0;
    unsigned lambda = 0;

    void init(unsigned secpar_bits) {
        lambda = secpar_bits;
        buf.clear();
        finalized = false;
        out.clear();
        out_used = 0;
    }

    void update(const void* data, size_t len) {
        if (finalized) return;
        buf.insert(buf.end(), (const uint8_t*)data, (const uint8_t*)data + len);
    }

    void finalize() {
        finalized = true;
    }

    void squeeze(void* dst, size_t len) {
        if (!finalized) finalize();
        size_t need = out_used + len;
        if (need > out.size()) {
            size_t old = out.size();
            out.resize(need);
            /* regenerate the full window (pseudoXOF is deterministic in msg+len) */
            pseudoXOF((unsigned long long)need * 8,
                      buf.data(), (unsigned long long)buf.size() * 8,
                      out.data());
        }
        memcpy(dst, out.data() + out_used, len);
        out_used += len;
    }

    void clear() {
        buf.clear();
        out.clear();
        out_used = 0;
        finalized = false;
    }
};

} // namespace faest

#endif
