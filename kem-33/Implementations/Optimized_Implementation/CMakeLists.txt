cmake_minimum_required(VERSION 3.10)
project(qube)

enable_testing()
enable_language(C)
set(CMAKE_C_STANDARD 99)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-incompatible-pointer-types)
endif()
set(QUBE_MTUNE_FLAG -mtune=native)
if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64" AND CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
    set(QUBE_MTUNE_FLAG -mtune=x86-64)
endif()
add_compile_options(-O3 -march=x86-64
-mavx2 ${QUBE_MTUNE_FLAG} -flto -fomit-frame-pointer -std=c99 -Wpedantic
-Wall -Wextra -mavx -mavx2 -mbmi -mpclmul)

# 目录
set(ROOT_DIR          ${CMAKE_CURRENT_SOURCE_DIR})
set(LIB_DIR           ${ROOT_DIR}/lib)
set(SRC_DIR           ${ROOT_DIR}/src)
set(TEST_DIR          ${ROOT_DIR}/tests)
set(BENCHMARK_DIR     ${ROOT_DIR}/benchmark)
set(BIN_DIR           ${CMAKE_BINARY_DIR}/bin)

# 公共头文件路径
include_directories(
    ${LIB_DIR}/fips202
    ${LIB_DIR}/api_pkc
    ${LIB_DIR}/bitpolymul_x86
    ${SRC_DIR}/common
    ${BENCHMARK_DIR}
)


# 公共源文件
file(GLOB COMMON_SRCS
    ${SRC_DIR}/common/*.c
)

file(GLOB X86_SRCS
    ${SRC_DIR}/x86/*.c
)

file(GLOB BITPOLYMUL_SRCS
    ${LIB_DIR}/bitpolymul_x86/*.c
)

file(GLOB API_PKC_SRCS
    ${LIB_DIR}/api_pkc/*.c
)

set(SHA3_SRC ${LIB_DIR}/fips202/fips202.c)

# 平台相关
set(PLATFORM_OBJ "")
if(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" AND EXISTS "${BENCHMARK_DIR}/hal.c")
        add_library(hal OBJECT ${BENCHMARK_DIR}/hal.c)
        target_include_directories(hal PRIVATE ${BENCHMARK_DIR})
        set(PLATFORM_OBJ $<TARGET_OBJECTS:hal>)
    endif()
endif()
if(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64" AND EXISTS "${BENCHMARK_DIR}/m1cycles.c")
        add_library(m1cycles OBJECT ${BENCHMARK_DIR}/m1cycles.c)
        target_include_directories(m1cycles PRIVATE ${BENCHMARK_DIR})
        set(PLATFORM_OBJ $<TARGET_OBJECTS:m1cycles>)
    endif()
endif()

# ------------------------------
# 自动构建 qube-opt 1/3/4/5
# ------------------------------
foreach(PROJ qube-1 qube-3 qube-4 qube-5)
    string(REPLACE "qube-" "" num ${PROJ})

    set(PROJ_DIR ${SRC_DIR}/x86/${PROJ})
    file(GLOB PROJ_SRCS ${PROJ_DIR}/*.c)

    set(PROJ_INCLUDES
        ${PROJ_DIR}
        ${SRC_DIR}/common
        ${SRC_DIR}/x86
        ${LIB_DIR}/fips202
        ${LIB_DIR}/api_pkc
        ${LIB_DIR}/bitpolymul_x86
        ${BENCHMARK_DIR}
    )

    # 核心库
    add_library(core_opt_${num} OBJECT
        ${COMMON_SRCS}
        ${X86_SRCS}
        ${API_PKC_SRCS}
        ${BITPOLYMUL_SRCS}
        ${SHA3_SRC}
        ${PROJ_SRCS}
        ${PLATFORM_OBJ}
    )

    target_include_directories(core_opt_${num}
        PRIVATE ${PROJ_INCLUDES}
    )

    # qube-opt-${num}
    add_executable(qube-opt-${num}
        ${TEST_DIR}/qube-test.c
    )

    target_include_directories(qube-opt-${num}
        PRIVATE ${PROJ_INCLUDES}
    )

    target_link_libraries(qube-opt-${num}
        PRIVATE core_opt_${num}
    )

    set_target_properties(qube-opt-${num}
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
    )

    # kat-opt-${num}
    add_executable(kat-opt-${num}
        ${TEST_DIR}/kat_kem.c
    )

    target_include_directories(kat-opt-${num}
        PRIVATE ${PROJ_INCLUDES}
    )

    target_link_libraries(kat-opt-${num}
        PRIVATE core_opt_${num}
    )

    set_target_properties(kat-opt-${num}
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
    )

    add_executable(benchmark-kem-${num}
        ${BENCHMARK_DIR}/benchmark_kem.c
    )

    target_include_directories(benchmark-kem-${num}
        PRIVATE ${PROJ_INCLUDES}
    )

    target_link_libraries(benchmark-kem-${num}
        PRIVATE core_opt_${num}
    )

    set_target_properties(benchmark-kem-${num}
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
    )

endforeach()

