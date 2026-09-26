================================================================================
  FEILIAN Hash Algorithm — Implementation Directory Structure
  Next-generation Commercial Cryptographic Algorithms Program (NGCC)
================================================================================

Directory Layout
----------------

Implementations/
├── README.txt                           (this file)
├── Reference_Implementation/            Scalar (portable C99) implementations
│   ├── FEILIAN1024/                     1024-bit digest, scalar
│   ├── FEILIAN512/                      512-bit digest, scalar
│   └── FEILIAN768/                      768-bit digest, scalar
├── Optimized_Implementation/            AVX2 SIMD implementations
│   ├── FEILIAN_SIMD1024/                1024-bit digest, AVX2 SIMD
│   ├── FEILIAN_SIMD512/                 512-bit digest, AVX2 SIMD
│   └── FEILIAN_SIMD768/                 768-bit digest, AVX2 SIMD
└── Additional_Implementation/           Hardware (SystemVerilog) implementations
    ├── FEILIAN_8SC/                     8SC Architecture, 1024-bit digest
    ├── FEILIAN_4SC/                     4SC Architecture, 512-bit digest
    ├── FEILIAN_2SC/                     2SC Architecture, 1024-bit digest
    └── FEILIAN_1SC/                     1SC Architecture, 512-bit digest


File Descriptions (per Algorithm Instance)
------------------------------------------

Each algorithm instance folder contains the following 5 files:

1. CryptHash_AlgorithmInstance.h
   Header file defining the programming interface.
   Macros to configure:
     - OUTPUT_BLANK_TEST_VECTORS : 0 = generate test vectors, 1 = blank template
     - ALGORITHM_INSTANCE        : algorithm instance name
     - DIGEST_BIT_LENGTH         : message digest length in bits
   Declares:
     int CryptHash(int digest_len_bits, const unsigned char *msg,
                   unsigned long long msg_len_bits, unsigned char *digest);

2. CryptHash_AlgorithmInstance.c
   Source file implementing the CryptHash() function for the FEILIAN algorithm.
   Contains the complete hash algorithm: message padding, 4x4 matrix state,
   20-round compression function (5 groups of 4 rounds), and output truncation.

3. KAT_CryptHash.c
   Known Answer Test (KAT) generator. SHALL NOT be modified.
   Generates four test vector files per instance:
     - KAT_2_12_[Instance].txt   : messages of lengths 0 to 2^12 bits
     - KAT_2_23_[Instance].txt   : all-0 / all-1 / random messages of 2^23 bits
     - KAT_2_33_[Instance].txt   : all-0 / all-1 / random messages of 2^33 bits
     - KAT_Loop_[Instance].txt   : 1,000,000-iteration loop test (2^13 bits)

4. drng.c
   Deterministic Random Number Generator based on SM3.
   Implements init_random_number() and get_random_number().
   SHALL NOT be modified.

5. drng.h
   Header file for the DRNG. Defines DRNG_ctx structure and declares:
     - int init_random_number(DRNG_ctx *, const unsigned char *, unsigned long long)
     - int get_random_number(DRNG_ctx *, unsigned char *, unsigned long long)
   SHALL NOT be modified.

Each hardware implementation folder contains the following 5 files:

1. feilian_pkg.sv
   Top-level SystemVerilog package defining the parameters and helper functions.

2. feilian.sv
   Top-level SystemVerilog module implementing the CryptHash() function for the FEILIAN algorithm.
   Contains the complete hash algorithm: message padding, 4x4 matrix state,
   20-round compression function (5 groups of 4 rounds), and output truncation.

3. core.sv
   SystemVerilog module implementing the core FEILIAN hash function.

4. compression.sv
   SystemVerilog module implementing the FEILIAN compression function.

5. round.sv
   SystemVerilog module implementing a single round of the FEILIAN compression function.

6. subcolumn.sv
   SystemVerilog module implementing a single SubColumn unit of the FEILIAN compression function.


Algorithm Variants Summary
---------------------------

| Instance Name      | Digest (bits) | Version  | Type          |
|--------------------|---------------|----------|---------------|
| FEILIAN1024        | 1024          | 0x400    | Scalar (C99)  |
| FEILIAN512         | 512           | 0x200    | Scalar (C99)  |
| FEILIAN768         | 768           | 0x300    | Scalar (C99)  |
| FEILIAN_SIMD1024   | 1024          | 0x400    | AVX2 SIMD     |
| FEILIAN_SIMD512    | 512           | 0x200    | AVX2 SIMD     |
| FEILIAN_SIMD768    | 768           | 0x300    | AVX2 SIMD     |

All variants share the same core FEILIAN algorithm:
- 1024-bit block size, 4x4 64-bit matrix state (2048 bits internal)
- 20 compression rounds (5 groups x 4 rounds) with BLAKE2-inspired S-box
- 128-bit message-length counter (max message length = 2^128 bits)
- 512/768-bit variants truncate the 1024-bit output from the MSB side


Compilation
-----------

Scalar (Reference):
  gcc -O2 -o kat.exe KAT_CryptHash.c CryptHash_AlgorithmInstance.c drng.c

SIMD (Optimized, requires AVX2):
  gcc -O2 -mavx2 -o kat.exe KAT_CryptHash.c CryptHash_AlgorithmInstance.c drng.c

Note: This program requires at least 1 GB of memory for KAT_2_33 (2^33-bit messages).


Test Vector Generation
----------------------

Run the compiled executable to generate test vector files in an "output/"
subdirectory. Move the generated .txt files into the Test_Vectors/ folder
alongside this Implementations/ directory.
