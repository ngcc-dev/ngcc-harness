cmake_minimum_required(VERSION 3.16)
project(ngcc_loom_reference_replay C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  add_compile_options(
    -Wno-error=implicit-function-declaration
    -Wno-error=incompatible-pointer-types
  )
endif()

get_filename_component(NGCC_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(LOOM_REF_ROOT
    "${NGCC_ROOT}/kex-05/Implementations and Test_Vectors/Implementations/Reference_Implementation"
    CACHE PATH "Loom reference implementation root")
set(LOOM_USE_SHAKE OFF CACHE BOOL "Use ICCS-compatible KAT path" FORCE)

add_subdirectory("${LOOM_REF_ROOT}" loom-submission EXCLUDE_FROM_ALL)

function(add_loom_replay LEVEL MODE KEM_TARGET SIG_TARGET)
  set(INSTANCE_ROOT "${LOOM_REF_ROOT}/LoomKEX-${LEVEL}")
  add_executable(loom_failure_replay_${LEVEL}
    "${NGCC_ROOT}/security/loom_failure_search.c"
    "${INSTANCE_ROOT}/loom/KEX_LoomKEX-${LEVEL}.c"
    "${INSTANCE_ROOT}/loom/loom.c"
    "${INSTANCE_ROOT}/loom/prf_mac.c"
    "${INSTANCE_ROOT}/loom/state_serialize.c"
    "${INSTANCE_ROOT}/loom/drng.c"
    "${INSTANCE_ROOT}/loom/auxfunc.c"
    "${INSTANCE_ROOT}/loom/symmetric-iccs.c"
  )
  target_include_directories(loom_failure_replay_${LEVEL} PRIVATE
    "${NGCC_ROOT}/api"
    "${INSTANCE_ROOT}/loom"
    "${INSTANCE_ROOT}/kem"
    "${INSTANCE_ROOT}/sig"
  )
  target_compile_definitions(loom_failure_replay_${LEVEL} PRIVATE
    LOOM_MODE=${MODE}
    LOOM_SEARCH_INSTANCE="LoomKEX-${LEVEL}"
    LOOM_SEARCH_IMPLEMENTATION="reference-kat-compatible"
    LOOM_SEARCH_HEADER="KEX_LoomKEX-${LEVEL}.h"
  )
  target_compile_options(loom_failure_replay_${LEVEL} PRIVATE -O3)
  target_link_libraries(loom_failure_replay_${LEVEL} PRIVATE ${KEM_TARGET} ${SIG_TARGET} m)
endfunction()

add_loom_replay(128 1 kem1_kat sig1_kat)
add_loom_replay(256 3 kem3_kat sig3_kat)
add_loom_replay(512 5 kem5_kat sig5_kat)

set(LOOM_256_ROOT "${LOOM_REF_ROOT}/LoomKEX-256")
add_executable(loom_state_rollback_key_recovery_256
  "${NGCC_ROOT}/security/loom_state_rollback_key_recovery.c"
  "${LOOM_256_ROOT}/loom/KEX_LoomKEX-256.c"
  "${LOOM_256_ROOT}/loom/loom.c"
  "${LOOM_256_ROOT}/loom/prf_mac.c"
  "${LOOM_256_ROOT}/loom/state_serialize.c"
  "${LOOM_256_ROOT}/loom/drng.c"
  "${LOOM_256_ROOT}/loom/auxfunc.c"
  "${LOOM_256_ROOT}/loom/symmetric-iccs.c"
)
target_include_directories(loom_state_rollback_key_recovery_256 PRIVATE
  "${NGCC_ROOT}/api"
  "${LOOM_256_ROOT}/loom"
  "${LOOM_256_ROOT}/kem"
  "${LOOM_256_ROOT}/sig"
)
target_compile_definitions(loom_state_rollback_key_recovery_256 PRIVATE LOOM_MODE=3)
target_compile_options(loom_state_rollback_key_recovery_256 PRIVATE -O3)
target_link_libraries(loom_state_rollback_key_recovery_256 PRIVATE kem3_kat sig3_kat m)
