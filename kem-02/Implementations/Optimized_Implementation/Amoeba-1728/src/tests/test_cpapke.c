#include "cpapke.h"
#include "util.h"

#ifdef PROFILING_ENABLE
#include <gperftools/profiler.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif

int main() {
    GetFrequency();

    printf("CPA SK size = %d\n", RLWE_CPA_SK_LEN);
    printf("CPA PK size = %d\n", RLWE_CPA_PK_LEN);
    printf("CPA CT size = %d\n", RLWE_CPA_CT_LEN);

    CPAPKE_Init();

    printf("\nCorrectness Test: \n");

    uint8_t coin[RLWE_SEED_LEN] = {1, 2, 4, 8};
    uint8_t sk[RLWE_CPA_SK_LEN] = {0};
    uint8_t pk[RLWE_CPA_PK_LEN] = {0};
    uint8_t ct[RLWE_CPA_CT_LEN] = {0};
    uint8_t pt1[RLWE_MSG_LEN] = {0};
    uint8_t pt2[RLWE_MSG_LEN] = {0};

    srand(clock());
    for(int i=0;i<RLWE_MSG_LEN;i++){
        pt1[i] = rand();
    }

    CPAPKE_KeyGen(sk, pk);
    CPAPKE_Encrypt(ct, pk, pt1, coin);
    CPAPKE_Decrypt(pt2, sk, ct);

    printf("pt1[%d] = ", RLWE_MSG_LEN);
    print_bytes(pt1, RLWE_MSG_LEN);

    printf("pt2[%d] = ", RLWE_MSG_LEN);
    print_bytes(pt2, RLWE_MSG_LEN);

    for(int i=0;i<RLWE_MSG_LEN;i++){
        if(pt1[i] != pt2[i]){
            printf("[ERROR] CPAPKE_Decrypt failed\n");
            break;
        }
    }

    printf("\nPerformance Test: \n");
    int loop = 10000;

#ifdef PROFILING_ENABLE
    ProfilerStart(PROFILE_PATH "TEST_CPAPKE_" STR(SECURITY_LEVEL) "_CPAPKE_KeyGen.prof");
#endif
    Loop(loop, { CPAPKE_KeyGen(sk, pk); }, "CPAPKE_KeyGen");
#ifdef PROFILING_ENABLE
    ProfilerStop();
#endif

#ifdef PROFILING_ENABLE
    ProfilerStart(PROFILE_PATH "TEST_CPAPKE_" STR(SECURITY_LEVEL) "_CPAPKE_Encrypt.prof");
#endif
    Loop(loop, { CPAPKE_Encrypt(ct, pk, pt1, coin); }, "CPAPKE_Encrypt");
#ifdef PROFILING_ENABLE
    ProfilerStop();
#endif

#ifdef PROFILING_ENABLE
    ProfilerStart(PROFILE_PATH "TEST_CPAPKE_" STR(SECURITY_LEVEL) "_CPAPKE_Decrypt.prof");
#endif
    Loop(loop, { CPAPKE_Decrypt(pt2, sk, ct); }, "CPAPKE_Decrypt");
#ifdef PROFILING_ENABLE
    ProfilerStop();
#endif

    return 0;
}