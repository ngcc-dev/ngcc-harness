# KAT and test_kex_state always use the ICCS submission path (drng + symmetric-iccs),
# independent of LOOM_USE_SHAKE on the kem/sig libraries.

set(KAT_LOOM_SRCS
  KAT_KEX.c
  KEX_LoomKEX-512.c
  loom.c
  prf_mac.c
  state_serialize.c
  drng.c
  auxfunc.c
  symmetric-iccs.c
)

add_executable(KAT_KEX_LoomKEX-512 ${KAT_LOOM_SRCS})

add_executable(test_kex_state_LoomKEX-512
  test_kex_state.c
  KEX_LoomKEX-512.c
  loom.c
  prf_mac.c
  state_serialize.c
  drng.c
  auxfunc.c
  symmetric-iccs.c
)

target_include_directories(KAT_KEX_LoomKEX-512
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/kem
    ${PROJECT_SOURCE_DIR}/sig
)

target_compile_definitions(KAT_KEX_LoomKEX-512 PUBLIC LOOM_MODE=${LOOM_MODE})
target_link_libraries(KAT_KEX_LoomKEX-512 PRIVATE kem5_kat sig5_kat)

target_include_directories(test_kex_state_LoomKEX-512
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/kem
    ${PROJECT_SOURCE_DIR}/sig
)

target_compile_definitions(test_kex_state_LoomKEX-512 PUBLIC LOOM_MODE=${LOOM_MODE})
target_link_libraries(test_kex_state_LoomKEX-512 PRIVATE kem5_kat sig5_kat)
