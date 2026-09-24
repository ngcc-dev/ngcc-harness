#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include <stddef.h>
// 引入您自己的参数和签名头文件
#include "params.h" 
#include "sign.h"   

// 官方定义的全局 DRNG 上下文，KAT 程序会用种子初始化它
extern DRNG_ctx drng_algorithm; 

unsigned long long sig_get_pk_len_bytes() {
    return CRYPTO_PUBLICKEYBYTES; // 替换为您的公钥宏
}

unsigned long long sig_get_sk_len_bytes() {
    return CRYPTO_SECRETKEYBYTES; // 替换为您的私钥宏
}

unsigned long long sig_get_sn_len_bytes() {
    return CRYPTO_BYTES; // 替换为您的签名长度宏
}

int sig_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    // 调用您的公私钥生成函数
    int ret = crypto_sign_keypair(pk, sk);
    *pk_len_bytes = CRYPTO_PUBLICKEYBYTES;
    *sk_len_bytes = CRYPTO_SECRETKEYBYTES;
    return ret;
}

int sig_sign(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes,
    unsigned char *sn, unsigned long long *sn_len_bytes)
{
    // 注意：官方 KAT 测试不需要 Context，所以传 NULL 和 0
    size_t out_len;
    int ret = crypto_sign_signature(sn, &out_len, m, m_len_bytes, NULL, 0, sk);
    *sn_len_bytes = out_len;
    return ret;
}

int sig_verify(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *sn, unsigned long long sn_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes)
{
    // 调用您的验签函数，如果验签成功返回 0，失败返回 -1
    return crypto_sign_verify(sn, sn_len_bytes, m, m_len_bytes, NULL, 0, pk);
}