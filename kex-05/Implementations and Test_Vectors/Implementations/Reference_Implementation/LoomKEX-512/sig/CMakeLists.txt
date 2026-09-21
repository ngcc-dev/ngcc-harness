set(_sig5_sources
  rounding.c
  sign.c
  polyvec.c
  sampler.c
  sampler_u.c
  irs.c
  packing.c
  poly.c
  poly_ntt.c
  reduce.c
  rans.c
  approx_exp.c
  approx_log.c
  ${SHUTTLE_QSETDIR}/ntt_ref.c
)

add_library(sig5 STATIC ${_sig5_sources})
add_library(sig5_kat STATIC ${_sig5_sources})

foreach(_sig_tgt sig5 sig5_kat)
  target_include_directories(${_sig_tgt}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${LOOM_512_SUBDIR}
      ${LOOM_512_SUBDIR}/loom
  )
  target_compile_definitions(${_sig_tgt}
    PUBLIC LOOM_MODE=${LOOM_MODE}
  )
endforeach()

if(_loom_use_shake_enabled)
  target_compile_definitions(sig5 PRIVATE LOOM_USE_SHAKE)
endif()
