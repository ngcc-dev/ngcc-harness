set(_kem5_sources
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

add_library(kem5 STATIC ${_kem5_sources})
add_library(kem5_kat STATIC ${_kem5_sources})

foreach(_kem_tgt kem5 kem5_kat)
  target_include_directories(${_kem_tgt}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${LOOM_512_SUBDIR}
      ${LOOM_512_SUBDIR}/loom
      ${LOOM_512_SUBDIR}/sig
  )
  target_compile_definitions(${_kem_tgt}
    PUBLIC LOOM_MODE=${LOOM_MODE}
  )
endforeach()

if(_loom_use_shake_enabled)
  target_compile_definitions(kem5 PRIVATE LOOM_USE_SHAKE)
endif()
