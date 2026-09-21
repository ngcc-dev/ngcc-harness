set(_kem1_sources
  cbd.c
  indcpa.c
  kem_cpaf.c
  msgenc.c
  ntt.c
  poly.c
  poly_invq.c
  polyvec.c
  reduce.c
  verify.c
)

add_library(kem1 STATIC ${_kem1_sources})
add_library(kem1_kat STATIC ${_kem1_sources})

foreach(_kem_tgt kem1 kem1_kat)
  target_include_directories(${_kem_tgt}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${LOOM_128_SUBDIR}
      ${LOOM_128_SUBDIR}/loom
      ${LOOM_128_SUBDIR}/sig
  )
  target_compile_definitions(${_kem_tgt}
    PUBLIC LOOM_MODE=${LOOM_MODE}
  )
endforeach()

if(_loom_use_shake_enabled)
  target_compile_definitions(kem1 PRIVATE LOOM_USE_SHAKE)
endif()
