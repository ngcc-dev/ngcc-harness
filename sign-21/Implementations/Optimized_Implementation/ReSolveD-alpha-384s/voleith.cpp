#include "voleith.inc"

namespace sig
{

// clang-format off








template bool sig_unpack_secret_key(secret_key<resolved_alpha::resolved_alpha_160_s>*, const uint8_t*);
template bool sig_unpack_secret_key(secret_key<resolved_alpha::resolved_alpha_160_f>*, const uint8_t*);
template bool sig_unpack_secret_key(secret_key<resolved_alpha::resolved_alpha_256_s>*, const uint8_t*);
template bool sig_unpack_secret_key(secret_key<resolved_alpha::resolved_alpha_256_f>*, const uint8_t*);
template bool sig_unpack_secret_key(secret_key<resolved_alpha::resolved_alpha_384_s>*, const uint8_t*);
template bool sig_unpack_secret_key(secret_key<resolved_alpha::resolved_alpha_384_f>*, const uint8_t*);
template bool sig_unpack_secret_key(secret_key<resolved_alpha::resolved_alpha_512_s>*, const uint8_t*);
template bool sig_unpack_secret_key(secret_key<resolved_alpha::resolved_alpha_512_f>*, const uint8_t*);

template void sig_pack_public_key(uint8_t*, const public_key<resolved_alpha::resolved_alpha_160_s>*);
template void sig_pack_public_key(uint8_t*, const public_key<resolved_alpha::resolved_alpha_160_f>*);
template void sig_pack_public_key(uint8_t*, const public_key<resolved_alpha::resolved_alpha_256_s>*);
template void sig_pack_public_key(uint8_t*, const public_key<resolved_alpha::resolved_alpha_256_f>*);
template void sig_pack_public_key(uint8_t*, const public_key<resolved_alpha::resolved_alpha_384_s>*);
template void sig_pack_public_key(uint8_t*, const public_key<resolved_alpha::resolved_alpha_384_f>*);
template void sig_pack_public_key(uint8_t*, const public_key<resolved_alpha::resolved_alpha_512_s>*);
template void sig_pack_public_key(uint8_t*, const public_key<resolved_alpha::resolved_alpha_512_f>*);

template void sig_unpack_public_key(public_key<resolved_alpha::resolved_alpha_160_s>*, const uint8_t*);
template void sig_unpack_public_key(public_key<resolved_alpha::resolved_alpha_160_f>*, const uint8_t*);
template void sig_unpack_public_key(public_key<resolved_alpha::resolved_alpha_256_s>*, const uint8_t*);
template void sig_unpack_public_key(public_key<resolved_alpha::resolved_alpha_256_f>*, const uint8_t*);
template void sig_unpack_public_key(public_key<resolved_alpha::resolved_alpha_384_s>*, const uint8_t*);
template void sig_unpack_public_key(public_key<resolved_alpha::resolved_alpha_384_f>*, const uint8_t*);
template void sig_unpack_public_key(public_key<resolved_alpha::resolved_alpha_512_s>*, const uint8_t*);
template void sig_unpack_public_key(public_key<resolved_alpha::resolved_alpha_512_f>*, const uint8_t*);

template bool sig_seckey<resolved_alpha::resolved_alpha_160_s>(const uint8_t*);
template bool sig_seckey<resolved_alpha::resolved_alpha_160_f>(const uint8_t*);
template bool sig_seckey<resolved_alpha::resolved_alpha_256_s>(const uint8_t*);
template bool sig_seckey<resolved_alpha::resolved_alpha_256_f>(const uint8_t*);
template bool sig_seckey<resolved_alpha::resolved_alpha_384_s>(const uint8_t*);
template bool sig_seckey<resolved_alpha::resolved_alpha_384_f>(const uint8_t*);
template bool sig_seckey<resolved_alpha::resolved_alpha_512_s>(const uint8_t*);
template bool sig_seckey<resolved_alpha::resolved_alpha_512_f>(const uint8_t*);

template bool sig_pubkey<resolved_alpha::resolved_alpha_160_s>(uint8_t*, const uint8_t*);
template bool sig_pubkey<resolved_alpha::resolved_alpha_160_f>(uint8_t*, const uint8_t*);
template bool sig_pubkey<resolved_alpha::resolved_alpha_256_s>(uint8_t*, const uint8_t*);
template bool sig_pubkey<resolved_alpha::resolved_alpha_256_f>(uint8_t*, const uint8_t*);
template bool sig_pubkey<resolved_alpha::resolved_alpha_384_s>(uint8_t*, const uint8_t*);
template bool sig_pubkey<resolved_alpha::resolved_alpha_384_f>(uint8_t*, const uint8_t*);
template bool sig_pubkey<resolved_alpha::resolved_alpha_512_s>(uint8_t*, const uint8_t*);
template bool sig_pubkey<resolved_alpha::resolved_alpha_512_f>(uint8_t*, const uint8_t*);

template bool voleith_sign<resolved_alpha::resolved_alpha_160_s>(uint8_t*, const uint8_t*, size_t, const uint8_t*, const uint8_t*, size_t);
template bool voleith_sign<resolved_alpha::resolved_alpha_160_f>(uint8_t*, const uint8_t*, size_t, const uint8_t*, const uint8_t*, size_t);
template bool voleith_sign<resolved_alpha::resolved_alpha_256_s>(uint8_t*, const uint8_t*, size_t, const uint8_t*, const uint8_t*, size_t);
template bool voleith_sign<resolved_alpha::resolved_alpha_256_f>(uint8_t*, const uint8_t*, size_t, const uint8_t*, const uint8_t*, size_t);
template bool voleith_sign<resolved_alpha::resolved_alpha_384_s>(uint8_t*, const uint8_t*, size_t, const uint8_t*, const uint8_t*, size_t);
template bool voleith_sign<resolved_alpha::resolved_alpha_384_f>(uint8_t*, const uint8_t*, size_t, const uint8_t*, const uint8_t*, size_t);
template bool voleith_sign<resolved_alpha::resolved_alpha_512_s>(uint8_t*, const uint8_t*, size_t, const uint8_t*, const uint8_t*, size_t);
template bool voleith_sign<resolved_alpha::resolved_alpha_512_f>(uint8_t*, const uint8_t*, size_t, const uint8_t*, const uint8_t*, size_t);

template bool voleith_verify<resolved_alpha::resolved_alpha_160_s>(const uint8_t*, const uint8_t*, size_t, const uint8_t*);
template bool voleith_verify<resolved_alpha::resolved_alpha_160_f>(const uint8_t*, const uint8_t*, size_t, const uint8_t*);
template bool voleith_verify<resolved_alpha::resolved_alpha_256_s>(const uint8_t*, const uint8_t*, size_t, const uint8_t*);
template bool voleith_verify<resolved_alpha::resolved_alpha_256_f>(const uint8_t*, const uint8_t*, size_t, const uint8_t*);
template bool voleith_verify<resolved_alpha::resolved_alpha_384_s>(const uint8_t*, const uint8_t*, size_t, const uint8_t*);
template bool voleith_verify<resolved_alpha::resolved_alpha_384_f>(const uint8_t*, const uint8_t*, size_t, const uint8_t*);
template bool voleith_verify<resolved_alpha::resolved_alpha_512_s>(const uint8_t*, const uint8_t*, size_t, const uint8_t*);
template bool voleith_verify<resolved_alpha::resolved_alpha_512_f>(const uint8_t*, const uint8_t*, size_t, const uint8_t*);

// clang-format on

} // namespace sig
