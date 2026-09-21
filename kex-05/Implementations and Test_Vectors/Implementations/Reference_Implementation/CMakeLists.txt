# CMakeLists.txt for LoomKEM Reference Implementation (Root)
# Builds all three security levels: 128, 256, 512
# Build: mkdir build && cd build && cmake .. && cmake --build .

cmake_minimum_required(VERSION 3.10)
project(LoomKEX-Reference C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set output directory for all executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Per-security-level LOOM mode defaults. These are independent so each
# subdirectory can be configured without overwriting the others.
set(LOOM_MODE_128 "1" CACHE STRING "LOOM mode for LoomKEX-128 (1, 3, or 5)")
set_property(CACHE LOOM_MODE_128 PROPERTY STRINGS "1" "3" "5")

set(LOOM_MODE_256 "3" CACHE STRING "LOOM mode for LoomKEX-256 (1, 3, or 5)")
set_property(CACHE LOOM_MODE_256 PROPERTY STRINGS "1" "3" "5")

set(LOOM_MODE_512 "5" CACHE STRING "LOOM mode for LoomKEX-512 (1, 3, or 5)")
set_property(CACHE LOOM_MODE_512 PROPERTY STRINGS "1" "3" "5")

# Add subdirectories for each security level
add_subdirectory(LoomKEX-128)
add_subdirectory(LoomKEX-256)
add_subdirectory(LoomKEX-512)

# Custom target: generate test vectors
add_custom_target(generate_kat
    COMMAND ./KAT_KEX_LoomKEX-128 && ./KAT_KEX_LoomKEX-256 && ./KAT_KEX_LoomKEX-512
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Generating KAT_KEX_LoomKEX-xxx.txt"
)

add_dependencies(generate_kat KAT_KEX_LoomKEX-128 KAT_KEX_LoomKEX-256 KAT_KEX_LoomKEX-512)

add_custom_target(test_kex_state
    DEPENDS test_kex_state_LoomKEX-128 test_kex_state_LoomKEX-256 test_kex_state_LoomKEX-512
    COMMENT "Run LSTA pack/unpack tests for all parameter sets"
)
