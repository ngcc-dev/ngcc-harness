/* sign-28: Algorithm 8's PRG output for lambda = 160, in the reference code's
 * 128-bit row layout. Inserted into a copy of src/vole_impl.inc ahead of
 * process_output_parts_round_keys, whose one call to eval_vole_round_key is
 * redirected here; nothing else in the build changes.
 *
 * Algorithm 8 (spec physical p. 19) returns s_0 || s_1 || ..., where s_c is the first
 * lambda bits of t_c = Encrypt(k, AddToLowerWord(iv', c) || 0^(kappa-lambda))
 * and iv' = AddToUpperWord(iv, twk). The shipped eval_vole_round_key(..., c)
 * computes exactly t_c's first lambda bits. Row `counter` of the tape (bytes
 * 16*counter .. 16*counter+15) therefore lies in s_first and s_(first+1),
 * first = floor(16*counter / 20). For lambda = 256 and 512 the shipped rows
 * already follow Algorithm 8, and the shipped function is used unchanged. */
static void spec_eval_vole_round_key(const sydo_ref_paramset_t* params, uint8_t* out,
                                     const aes_round_keys_t* round_key, const uint8_t* iv,
                                     uint32_t tweak, uint32_t counter) {
  uint8_t s[2][64];
  uint32_t first;

  if (params->secpar_bits != 160) {
    eval_vole_round_key(params, out, round_key, iv, tweak, counter);
    return;
  }
  first = (uint32_t)((16u * (uint64_t)counter) / 20u);
  eval_vole_round_key(params, s[0], round_key, iv, tweak, first);
  eval_vole_round_key(params, s[1], round_key, iv, tweak, first + 1u);
  for (size_t j = 0; j != 16u; ++j) {
    const size_t o = 16u * (size_t)counter + j - 20u * (size_t)first;
    out[j] = s[o / 20u][o % 20u];
  }
}

