set(_kem3_sources
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

add_library(kem3 STATIC ${_kem3_sources})
add_library(kem3_kat STATIC ${_kem3_sources})

foreach(_kem_tgt kem3 kem3_kat)
  target_include_directories(${_kem_tgt}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${LOOM_256_SUBDIR}
      ${LOOM_256_SUBDIR}/loom
      ${LOOM_256_SUBDIR}/sig
  )
  target_compile_definitions(${_kem_tgt}
    PUBLIC LOOM_MODE=${LOOM_MODE}
  )
endforeach()

if(_loom_use_shake_enabled)
  target_compile_definitions(kem3 PRIVATE LOOM_USE_SHAKE)
endif()
