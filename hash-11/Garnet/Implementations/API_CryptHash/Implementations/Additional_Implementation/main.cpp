#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include "hash_garnet.h"
#include <sys/stat.h>

// --- 填补系统调用缺失的桩函数 (Syscall Stubs) ---
#include <unistd.h>
#include "CryptHash_Garnet.h"

extern "C" {

    int _getentropy(void *buffer, size_t length) {
        for (size_t i = 0; i < length; ++i) ((char*)buffer)[i] = 0;
        return 0; 
    }

}
/**
 * @brief 辅助打印函数：将摘要以十六进制格式输出
 * @param title  标题字符串
 * @param digest 摘要缓冲区指针
 * @param bits   摘要的比特长度 (512, 768 或 1024)
 */
void print_digest(const char* title, const uint8_t* digest, int bits) {
    int bytes = bits / 8;
    std::cout << title << " (" << bits << " bits):" << std::endl;
    
    for (int i = 0; i < bytes; ++i) {
        // 设置 2 位十六进制，不足补 0
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
        
        // 每 32 字节换一行，方便阅读
        if ((i + 1) % 32 == 0) {
            std::cout << std::endl;
        }
    }
    // 恢复为十进制格式，避免影响后续输出
    std::cout << std::dec << "\n" << std::endl;
}
// ------------------------------------------------

// 【加入这里：告诉 C++ 这是纯 C 接口】
#ifdef __cplusplus
extern "C" {
#endif
void pad_message(unsigned char* msg, uint64_t bit_len);
// 【加入这里：闭合大括号】
#ifdef __cplusplus
}
#endif
// 辅助打印函数（带换行控制）
void print_hex(const char* label, const uint8_t* data, size_t length) {
    std::cout << label << " (" << std::dec << length * 8 << "-bit):" << std::endl;
    for (size_t i = 0; i < length; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
        if ((i + 1) % 16 == 0) std::cout << std::endl;
        else if ((i + 1) % 8 == 0) std::cout << " ";
    }
    // 如果最后一行没有凑够16个字节，补一个换行
    if (length % 16 != 0) std::cout << std::endl;
    std::cout << std::dec << std::endl;
}

int main() {
    std::cout << "=======================================" << std::endl;
    std::cout << " Garnet Universal Hash Engine Testbed  " << std::endl;
    std::cout << "=======================================\n" << std::endl;

    // 1. 设置想要测试的位长度 (对齐旧版逻辑)
    uint64_t bitlength = 477; // 也可以换成 136747 测长消息
    std::cout << "bitlength=" << bitlength << "\n" << std::endl;

    // 2. 计算需要的字节数并动态分配内存
    uint64_t byte_num = (bitlength % 8 == 0) ? (bitlength / 8) : (bitlength / 8 + 1);
    
    // [CRITICAL FIX]: 使用 vector 替代 calloc。
    // 务必追加 +16 字节的安全缓冲区！防止 C 底层处理尾块时跨界读取引发段错误或污染 Hash 状态。
    std::vector<uint8_t> message(byte_num + 16, 0); 

    // 3. 构造测试输入 (对齐旧版: 首字节为 0x01, 其余全为 0)
    if (byte_num > 0) {
        message[0] = 0x01; 
    }

    // 必须在这里调用原版的 pad_message 修改 buffer 的末尾比特，保证完全对齐原版行为
    pad_message(message.data(), bitlength);

    // 4. 打印原始输入消息 (只打印实际的 byte_num 长度)
    std::cout << "The original message is:" << std::endl;
    for (size_t i = 0; i < byte_num; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)message[i];
        if (((i + 1) % 16) == 0) std::cout << std::endl;
    }
    if (byte_num % 16 != 0) std::cout << std::endl;
    std::cout << std::endl;

    // 分配哈希输出缓冲区和状态矩阵
    uint8_t digest[128];
    struct uint128_t state[25];


    // =========================================================
    // 家族 1: Garnet-512 (共 5 种速率变体, 摘要长度 64 字节)
    // =========================================================
    
    // Test 1: Garnet-512 (Rate 512, 4x4)
    #ifdef ENABLE_GARNET_512_W512
        memset(digest, 0, sizeof(digest));
        for(int i = 0; i < 25; i++) zero128(state + i); 
        initialize_state_st(state, 512, COUNTER_512_W512);
        hash_garnet_universal_st(message.data(), bitlength, digest, state, COUNTER_512_W512);
        print_hex("Test 1: Garnet-512 (w=512) result:", digest, 64);
    #else
        std::cout << "[SKIP] Test 1: Garnet-512 (w=512) not enabled.\n" << std::endl;
    #endif

    // Test 2: Garnet-512 (Rate 640, 4x4)
    #ifdef ENABLE_GARNET_512_W640
        memset(digest, 0, sizeof(digest));
        for(int i = 0; i < 25; i++) zero128(state + i);
        initialize_state_st(state, 512, COUNTER_512_W640);
        hash_garnet_universal_st(message.data(), bitlength, digest, state, COUNTER_512_W640);
        print_hex("Test 2: Garnet-512 (w=640) result:", digest, 64);
    #else
        std::cout << "[SKIP] Test 2: Garnet-512 (w=640) not enabled.\n" << std::endl;
    #endif

    // Test 3: Garnet-512 (Rate 768, 4x4)
    #ifdef ENABLE_GARNET_512_W768
        memset(digest, 0, sizeof(digest));
        for(int i = 0; i < 25; i++) zero128(state + i);
        initialize_state_st(state, 512, COUNTER_512_W768);
        hash_garnet_universal_st(message.data(), bitlength, digest, state, COUNTER_512_W768);
        print_hex("Test 3: Garnet-512 (w=768) result:", digest, 64);
    #else
        std::cout << "[SKIP] Test 3: Garnet-512 (w=768) not enabled.\n" << std::endl;
    #endif

    // Test 4: Garnet-512 (Rate 896, 4x4)
    #ifdef ENABLE_GARNET_512_W896
        memset(digest, 0, sizeof(digest));
        for(int i = 0; i < 25; i++) zero128(state + i);
        initialize_state_st(state, 512, COUNTER_512_W896);
        hash_garnet_universal_st(message.data(), bitlength, digest, state, COUNTER_512_W896);
        print_hex("Test 4: Garnet-512 (w=896) result:", digest, 64);
    #else
        std::cout << "[SKIP] Test 4: Garnet-512 (w=896) not enabled.\n" << std::endl;
    #endif

    // Test 5: Garnet-512 (Rate 1024, 4x4)
    #ifdef ENABLE_GARNET_512_W1024
        memset(digest, 0, sizeof(digest));
        for(int i = 0; i < 25; i++) zero128(state + i);
        initialize_state_st(state, 512, COUNTER_512_W1024);
        hash_garnet_universal_st(message.data(), bitlength, digest, state, COUNTER_512_W1024);
        print_hex("Test 5: Garnet-512 (w=1024) result:", digest, 64);
    #else
        std::cout << "[SKIP] Test 5: Garnet-512 (w=1024) not enabled.\n" << std::endl;
    #endif


    // =========================================================
    // 家族 2: Garnet-768 (共 1 种速率变体, 摘要长度 96 字节)
    // =========================================================

    // Test 6: Garnet-768 (Rate 512, 4x4)
    #ifdef ENABLE_GARNET_768_W512
        memset(digest, 0, sizeof(digest));
        for(int i = 0; i < 25; i++) zero128(state + i);
        initialize_state_st(state, 768, COUNTER_768_W512);
        hash_garnet_universal_st(message.data(), bitlength, digest, state, COUNTER_768_W512);
        print_hex("Test 6: Garnet-768 (w=512) result:", digest, 96);
    #else
        std::cout << "[SKIP] Test 6: Garnet-768 (w=512) not enabled.\n" << std::endl;
    #endif


    // =========================================================
    // 家族 3: Garnet-1024 (共 2 种速率变体, 5x5 矩阵, 摘要长度 128 字节)
    // =========================================================

    // Test 7: Garnet-1024 (Rate 1024, 5x5 Matrix)
    #ifdef ENABLE_GARNET_1024_W1024
        memset(digest, 0, sizeof(digest));
        for(int i = 0; i < 25; i++) zero128(state + i);
        initialize_state_st(state, 1024, COUNTER_1024_W1024);
        hash_garnet_universal_st(message.data(), bitlength, digest, state, COUNTER_1024_W1024);
        print_hex("Test 7: Garnet-1024 (w=1024, 5x5) result:", digest, 128);
    #else
        std::cout << "[SKIP] Test 7: Garnet-1024 (w=1024) not enabled.\n" << std::endl;
    #endif

    // Test 8: Garnet-1024 (Rate 1152, 5x5 Matrix)
    #ifdef ENABLE_GARNET_1024_W1152
        memset(digest, 0, sizeof(digest));
        for(int i = 0; i < 25; i++) zero128(state + i);
        initialize_state_st(state, 1024, COUNTER_1024_W1152);
        hash_garnet_universal_st(message.data(), bitlength, digest, state, COUNTER_1024_W1152);
        print_hex("Test 8: Garnet-1024 (w=1152, 5x5) result:", digest, 128);
    #else
        std::cout << "[SKIP] Test 8: Garnet-1024 (w=1152) not enabled.\n" << std::endl;
    #endif


    // =========================================================
    // 家族 4: Garnet-1024a (共 1 种速率变体, 4x4 矩阵 Sponge-DM, 摘要长度 128 字节)
    // =========================================================

    // Test 9: Garnet-1024a (Sponge-DM, Rate 896, 4x4)
    #ifdef ENABLE_GARNET_1024A_SP_DM
        memset(digest, 0, sizeof(digest));
        for(int i = 0; i < 25; i++) zero128(state + i);
        initialize_state_st(state, 1024, COUNTER_1024A_SP_DM);
        hash_garnet_universal_st(message.data(), bitlength, digest, state, COUNTER_1024A_SP_DM);
        print_hex("Test 9: Garnet-1024a (Sponge-DM, w=896) result:", digest, 128);
    #else
        std::cout << "[SKIP] Test 9: Garnet-1024a (Sponge-DM) not enabled.\n" << std::endl;
    #endif

    // =========================================================
    // 家族 5: Garnet-1024 Flagship (5x5 矩阵 Sponge-DM, 2048-bit 速率)
    // =========================================================

    // Test 10: Garnet-1024 Flagship (Sponge-DM, Rate 2048, 5x5)
    #ifdef ENABLE_GARNET_1024_W2048_DM
        std::cout << "--- Flagship Variant Testing ---" << std::endl;
        memset(digest, 0, sizeof(digest));
        for(int i = 0; i < 25; i++) zero128(state + i);

        // 1. 初始化 5x5 旗舰版 IV (Counter: 0x8800100c100c0000)
        initialize_state_st(state, 1024, COUNTER_1024_W2048_DM);

        // 2. 调用通用哈希引擎 (内部会自动执行 Fig. 2 的 Double-XOR 逻辑)
        hash_garnet_universal_st(message.data(), bitlength, digest, state, COUNTER_1024_W2048_DM);

        // 3. 打印 1024 位 (128 字节) 摘要结果
        print_hex("Test 10: Garnet-1024 Flagship (w=2048, 5x5-DM) result:", digest, 128);
    #else
        std::cout << "[SKIP] Test 10: Garnet-1024 Flagship (w=2048) not enabled.\n" << std::endl;
    #endif

    std::cout << "===============================================" << std::endl;
    std::cout << "   Algorithm: " << ALGORITHM_INSTANCE << std::endl;
    std::cout << "===============================================\n" << std::endl;

    // --- 测试案例 1: 477 比特的非整字节消息 ---
    unsigned long long bitlen1 = 477;
    uint32_t bytelen1 = (bitlen1 + 7) / 8;
    std::vector<uint8_t> msg1(bytelen1, 0);
    msg1[0] = 0x01; // 构造测试数据

    uint8_t digest1[128]; // 足够容纳 1024 bit
    
    std::cout << "Testing Case 1: " << bitlen1 << " bits..." << std::endl;
    // 调用标准接口
    int ret1 = CryptHash(DIGEST_BIT_LENGTH, msg1.data(), bitlen1, digest1);
    
    if (ret1 == 0) {
        print_digest("Result", digest1, DIGEST_BIT_LENGTH);
    } else {
        std::cerr << "Error: CryptHash failed with code " << ret1 << std::endl;
    }




    // --- 测试案例 2: 长消息测试 (1024 bits) ---
    unsigned long long bitlen3 = 1024;
    std::vector<uint8_t> msg3(128, 0xAA); // 全 0xAA
    uint8_t digest3[128];

    std::cout << "Testing Case 3: 1024 bits block..." << std::endl;
    int ret3 = CryptHash(DIGEST_BIT_LENGTH, msg3.data(), bitlen3, digest3);
    
    if (ret3 == 0) {
        print_digest("Result", digest3, DIGEST_BIT_LENGTH);
    }

     // --- 测试案例 3: 768位摘要测试 (针对 Garnet-768) ---
    unsigned long long bitlen4 = 768; // 消息长度 768 bits (96 字节)
    std::vector<uint8_t> msg4(96, 0xBB); // 填充 0xBB
    uint8_t digest4[128]; // 足够大

    std::cout << "Testing Case 4: 768 bits message -> 768 bits Digest..." << std::endl;
    // 第一个参数传 768，强制触发 Garnet-768 逻辑 (4x4 矩阵)
    int ret4 = CryptHash(768, msg4.data(), bitlen4, digest4);
    
    if (ret4 == 0) {
        print_digest("Result Garnet-768", digest4, 768);
    } else {
        std::cerr << "Error: CryptHash 768 failed!" << std::endl;
    }


    // --- 测试案例 4: 1024位摘要测试 (针对 Garnet-1024) ---
    unsigned long long bitlen5 = 1024; // 消息长度 1024 bits (128 字节)
    std::vector<uint8_t> msg5(128, 0xCC); // 填充 0xCC
    uint8_t digest5[128];

    std::cout << "Testing Case 5: 1024 bits message -> 1024 bits Digest..." << std::endl;
    int ret5 = CryptHash(1024, msg5.data(), bitlen5, digest5);
    
    if (ret5 == 0) {
        print_digest("Result Garnet-1024", digest5, 1024);
    } else {
        std::cerr << "Error: CryptHash 1024 failed!" << std::endl;
    }

    std::cout << "===============================================" << std::endl;
    std::cout << "        All Standard Cases Completed.          " << std::endl;
    std::cout << "===============================================" << std::endl;


    return 0;
}