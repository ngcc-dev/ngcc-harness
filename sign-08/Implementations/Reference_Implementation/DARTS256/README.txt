*********************************Instructions**********************************

The following files SHALL NOT be modified:

drng.c                      Source file of Deterministic Random Number Generator

drng.h                      Header file of Deterministic Random Number Generator

auxfunc.c                   Source file of auxiliary functions

auxfunc.h                   Header file of auxiliary functions

KAT_SIG.c                   Source file for generating test vector files of 
                            digital signature (SIG) scheme

KAT_KEM.c                   Source file for generating test vector files of 
                            key encapsulation mechanism (KEM) scheme

KAT_KEX.c                   Source file for generating test vector files of 
                            key exchange (KEX) protocol

The following files NEED to be modified:

SIG_AlgorithmInstance.c     Source file of SIG scheme (demo)

SIG_AlgorithmInstance.h     Header file of SIG scheme (programming interface)

KEM_AlgorithmInstance.c     Source file of KEM scheme (demo)

KEM_AlgorithmInstance.h     Header file of KEM scheme (programming interface)

KEX_AlgorithmInstance.c     Source file of KEX protocol (demo)

KEX_AlgorithmInstance.h     Header file of KEX protocol (programming interface)

**************************************Use**************************************

1.  Modify the provided file "XXX_AlgorithmInstance.h":
    a.  Set the macro "OUTPUT_BLANK_TEST_VECTORS" as 0 to set generating mode.
    b.  Set the macro "ALGORITHM_INSTANCE" as the submitted algorithm instance 
        name.

2.  Modify the provided file "XXX_AlgorithmInstance.c" to implement the 
    functions of the submitted algorithm.

3.  Compile and execute to generate test vector files.

*************************************Notes*************************************

1.  The submitted algorithm shall use the provided programming interface in 
    "XXX_AlgorithmInstance.h".    
2.  This program assumes a little-endian byte order for multi-byte values. 
    Behavior is undefined on systems with a different byte order.
3.  This program requires compilation with compilers supporting the 
    ISO/IEC 9899:1999 (C99) standard or later.
4.  For non-byte-aligned data operations (e.g., DRNG outputs), the most
	significant bit (MSB) first convention applies to partial-byte read/write
    operations.
    Example: the 11-bit non-byte-aligned output "10101100001"(bin) is stored 
    in memory as "AC 20"(hex):
                     MSB     ------->     LSB         
        Address+0  :  1  0  1  0  1  1  0  0  (0xAC)
        Address+1  :  0  0  1  0  0  0  0  0  (0x20, with only 3 valid bits)
5.  The cryptographic hash algorithm and eXtendable-Output Function (XOF) in 
    implementations shall use the auxiliary functions. These auxiliary functions
    are only used to verify the correctness of implementations and preliminarily
    evaluate performance, without considering security. In the subsequent 
    evaluation rounds, these will be replaced with new auxiliary functions based
    on new cryptographic hash algorithms.
6.  The files of KEX are only suitable for 2-pass and 3-pass key exchange 
    protocols. If the submitted key exchange protocol requires more passes, 
    modify "KAT_KEX.c" and refer to the given programming interface to implement
    those passes.