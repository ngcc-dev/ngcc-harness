cmake_minimum_required(VERSION 3.10)
project(qube_128_reference C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LIB_DIR ${ROOT_DIR}/lib)
set(SRC_DIR ${ROOT_DIR}/src)
set(TEST_DIR ${ROOT_DIR}/test)
set(INSTANCE_SUFFIX qube_128)
set(INSTANCE_SECURITY 128)

set(COMMON_SRCS
    ${SRC_DIR}/common/code.c
    ${SRC_DIR}/common/crypto_memset.c
    ${SRC_DIR}/common/fft.c
    ${SRC_DIR}/common/kem.c
    ${SRC_DIR}/common/kem_qube.c
    ${SRC_DIR}/common/symmetric.c
)

set(REF_SRCS
    ${SRC_DIR}/ref/gf.c
    ${SRC_DIR}/ref/gf2x.c
    ${SRC_DIR}/ref/parsing.c
    ${SRC_DIR}/ref/qube.c
    ${SRC_DIR}/ref/reed_muller.c
    ${SRC_DIR}/ref/reed_solomon.c
    ${SRC_DIR}/ref/vector.c
)

set(API_PKC_SRCS
    ${LIB_DIR}/api_pkc/auxfunc.c
    ${LIB_DIR}/api_pkc/drng.c
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    enable_testing()
endif()

add_library(api_pkc_${INSTANCE_SUFFIX} OBJECT ${API_PKC_SRCS})
target_include_directories(api_pkc_${INSTANCE_SUFFIX} PRIVATE ${LIB_DIR}/api_pkc)

add_library(core_${INSTANCE_SUFFIX} OBJECT ${COMMON_SRCS} ${REF_SRCS})
target_include_directories(core_${INSTANCE_SUFFIX} PRIVATE
    ${SRC_DIR}/common
    ${SRC_DIR}/ref
)
target_include_directories(core_${INSTANCE_SUFFIX} SYSTEM PRIVATE ${LIB_DIR}/api_pkc)
target_compile_options(core_${INSTANCE_SUFFIX} PRIVATE -Wall -Wextra -Wpedantic)

function(add_qube_instance_test target source output_name)
    add_executable(${target}
        ${source}
        $<TARGET_OBJECTS:core_${INSTANCE_SUFFIX}>
        $<TARGET_OBJECTS:api_pkc_${INSTANCE_SUFFIX}>
    )
    target_include_directories(${target} PRIVATE
        ${SRC_DIR}/common
        ${SRC_DIR}/ref
        ${LIB_DIR}/api_pkc
    )
    set_target_properties(${target} PROPERTIES OUTPUT_NAME ${output_name})
endfunction()

add_qube_instance_test(test_${INSTANCE_SUFFIX} ${TEST_DIR}/test_kem.c test-qube-${INSTANCE_SECURITY})
add_test(NAME test-qube-${INSTANCE_SECURITY} COMMAND test_${INSTANCE_SUFFIX})

add_qube_instance_test(components_${INSTANCE_SUFFIX} ${TEST_DIR}/test_components.c components-qube-${INSTANCE_SECURITY})
add_test(NAME components-qube-${INSTANCE_SECURITY} COMMAND components_${INSTANCE_SUFFIX})

add_qube_instance_test(soak_${INSTANCE_SUFFIX} ${TEST_DIR}/test_soak.c soak-qube-${INSTANCE_SECURITY})
add_test(NAME soak-qube-${INSTANCE_SECURITY} COMMAND soak_${INSTANCE_SUFFIX})

add_qube_instance_test(kat_${INSTANCE_SUFFIX} ${TEST_DIR}/KAT_KEM.c kat-qube-${INSTANCE_SECURITY})
add_test(NAME kat-qube-${INSTANCE_SECURITY} COMMAND kat_${INSTANCE_SUFFIX})
set_tests_properties(kat-qube-${INSTANCE_SECURITY} PROPERTIES WORKING_DIRECTORY ${TEST_DIR})
