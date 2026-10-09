/* Own harness: includes the AXIS reference core (compiled by us) and exposes
   internal beat functions for cross-checking the Python model. */
#include "axis_core.c"

int w_hash(int variant, const uint8_t *msg, uint64_t bits, uint8_t *out) {
    axis_core_hash_bits((axis_variant_t)variant, msg, bits, out); return 0;
}
/* bit-by-bit (non-fast) path: force the generic path by using update_bits on a copy */
int w_hash_slow(int variant, const uint8_t *msg, uint64_t bits, uint8_t *out) {
    axis_core_t ctx; axis_core_init(&ctx, (axis_variant_t)variant);
    axis_reserve_message_bits(&ctx, bits + 8);
    for (uint64_t i = 0; i < bits; i++) axis_set_stored_message_bit(&ctx, i, axis_read_input_bit(msg, i));
    ctx.total_message_bits = bits;
    memset(out, 0, axis_result_bits((axis_variant_t)variant) / 8);
    axis_absorb_padded_message(&ctx); axis_release_message(&ctx); axis_emit_digest(&ctx, out);
    return 0;
}
void w_step(uint64_t *regs, int variant, uint8_t m0, uint8_t m1) {
    axis_core_t ctx; axis_core_init(&ctx, (axis_variant_t)variant);
    memcpy(ctx.regs, regs, sizeof(ctx.regs)); axis_step(&ctx, m0, m1); memcpy(regs, ctx.regs, sizeof(ctx.regs));
}
void w_reupfull(uint64_t *regs, uint8_t cbyte) {
    axis_core_t ctx; axis_core_init(&ctx, AXIS_VARIANT_1024);
    memcpy(ctx.regs, regs, sizeof(ctx.regs)); axis_reupfull(&ctx, cbyte); memcpy(regs, ctx.regs, sizeof(ctx.regs));
}
uint8_t w_cbyte(uint64_t padded_bits, uint64_t beat) { return axis_reupfull_constant_byte(padded_bits, beat); }
