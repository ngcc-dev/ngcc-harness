#include "ccakem.h"
#include "util.h"

#ifdef PROFILING_ENABLE
#include <gperftools/profiler.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif

int main() {
    GetFrequency();

    printf("CCA SK size = %d\n", RLWE_CCA_SK_LEN);
    printf("CCA PK size = %d\n", RLWE_CCA_PK_LEN);
    printf("CCA CT size = %d\n", RLWE_CCA_CT_LEN);

    CCAKEM_Init();

    printf("\nCorrectness Test: \n");

    uint8_t sk[RLWE_CCA_SK_LEN] = {0};
    uint8_t pk[RLWE_CCA_PK_LEN] = {0};
    uint8_t ct[RLWE_CCA_CT_LEN] = {0};
    uint8_t key1[RLWE_KEY_LEN] = {0};
    uint8_t key2[RLWE_KEY_LEN] = {0};

    CCAKEM_KeyGen(sk, pk);
    CCAKEM_Encaps(ct, key1, pk);
    CCAKEM_Decaps(key2, sk, ct);

    printf("key1[%d] = ", RLWE_KEY_LEN);
    print_bytes(key1, RLWE_KEY_LEN);

    printf("key2[%d] = ", RLWE_KEY_LEN);
    print_bytes(key2, RLWE_KEY_LEN);

    for(int i=0;i<RLWE_KEY_LEN;i++){
        if(key1[i] != key2[i]){
            printf("[ERROR] CCAKEM_Decaps failed\n");
            break;
        }
    }

    printf("\nPerformance Test: \n");
    int loop = 10000;

#ifdef PROFILING_ENABLE
    ProfilerStart(PROFILE_PATH "TEST_CCAKEM_" STR(SECURITY_LEVEL) "_CCAKEM_KeyGen.prof");
#endif
    Loop(loop, { CCAKEM_KeyGen(sk, pk); }, "CCAKEM_KeyGen");
#ifdef PROFILING_ENABLE
    ProfilerStop();
#endif

#ifdef PROFILING_ENABLE
    ProfilerStart(PROFILE_PATH "TEST_CCAKEM_" STR(SECURITY_LEVEL) "_CCAKEM_Encaps.prof");
#endif
    Loop(loop, { CCAKEM_Encaps(ct, key1, pk); }, "CCAKEM_Encaps");
#ifdef PROFILING_ENABLE
    ProfilerStop();
#endif

#ifdef PROFILING_ENABLE
    ProfilerStart(PROFILE_PATH "TEST_CCAKEM_" STR(SECURITY_LEVEL) "_CCAKEM_Decaps.prof");
#endif
    Loop(loop, { CCAKEM_Decaps(key2, sk, ct); }, "CCAKEM_Decaps");
#ifdef PROFILING_ENABLE
    ProfilerStop();
#endif

    return 0;
}