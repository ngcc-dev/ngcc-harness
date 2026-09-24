#include "vole_commit.inc"

namespace sig
{

// clang-format off

template void vole_commit<lynx::lynx_160_s>(block_secpar<lynx::lynx_160_s::secpar_v>, block128, block_secpar<lynx::lynx_160_s::secpar_v>* __restrict__, unsigned char* __restrict__, vole_block* __restrict__, vole_block* __restrict__, uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);
template void vole_commit<lynx::lynx_160_f>(block_secpar<lynx::lynx_160_f::secpar_v>, block128, block_secpar<lynx::lynx_160_f::secpar_v>* __restrict__, unsigned char* __restrict__, vole_block* __restrict__, vole_block* __restrict__, uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);
template void vole_commit<lynx::lynx_256_s>(block_secpar<lynx::lynx_256_s::secpar_v>, block128, block_secpar<lynx::lynx_256_s::secpar_v>* __restrict__, unsigned char* __restrict__, vole_block* __restrict__, vole_block* __restrict__, uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);
template void vole_commit<lynx::lynx_256_f>(block_secpar<lynx::lynx_256_f::secpar_v>, block128, block_secpar<lynx::lynx_256_f::secpar_v>* __restrict__, unsigned char* __restrict__, vole_block* __restrict__, vole_block* __restrict__, uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);
template void vole_commit<lynx::lynx_384_s>(block_secpar<lynx::lynx_384_s::secpar_v>, block128, block_secpar<lynx::lynx_384_s::secpar_v>* __restrict__, unsigned char* __restrict__, vole_block* __restrict__, vole_block* __restrict__, uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);
template void vole_commit<lynx::lynx_384_f>(block_secpar<lynx::lynx_384_f::secpar_v>, block128, block_secpar<lynx::lynx_384_f::secpar_v>* __restrict__, unsigned char* __restrict__, vole_block* __restrict__, vole_block* __restrict__, uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);
template void vole_commit<lynx::lynx_512_s>(block_secpar<lynx::lynx_512_s::secpar_v>, block128, block_secpar<lynx::lynx_512_s::secpar_v>* __restrict__, unsigned char* __restrict__, vole_block* __restrict__, vole_block* __restrict__, uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);
template void vole_commit<lynx::lynx_512_f>(block_secpar<lynx::lynx_512_f::secpar_v>, block128, block_secpar<lynx::lynx_512_f::secpar_v>* __restrict__, unsigned char* __restrict__, vole_block* __restrict__, vole_block* __restrict__, uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);

template bool vole_reconstruct<lynx::lynx_160_s>(block128, vole_block* __restrict__, const uint8_t*, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);
template bool vole_reconstruct<lynx::lynx_160_f>(block128, vole_block* __restrict__, const uint8_t*, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);
template bool vole_reconstruct<lynx::lynx_256_s>(block128, vole_block* __restrict__, const uint8_t*, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);
template bool vole_reconstruct<lynx::lynx_256_f>(block128, vole_block* __restrict__, const uint8_t*, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);
template bool vole_reconstruct<lynx::lynx_384_s>(block128, vole_block* __restrict__, const uint8_t*, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);
template bool vole_reconstruct<lynx::lynx_384_f>(block128, vole_block* __restrict__, const uint8_t*, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);
template bool vole_reconstruct<lynx::lynx_512_s>(block128, vole_block* __restrict__, const uint8_t*, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);
template bool vole_reconstruct<lynx::lynx_512_f>(block128, vole_block* __restrict__, const uint8_t*, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__);

// clang-format on

} // namespace sig
