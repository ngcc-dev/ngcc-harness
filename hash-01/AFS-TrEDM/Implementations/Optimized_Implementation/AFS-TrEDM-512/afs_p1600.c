/*
 * Optimized AFS-p-S6[1600,nr] implementation for AFS-TrEDM-S6.
 *
 * This optimized source differs from the reference implementation in three ways:
 *   1. round/lane constants are precomputed instead of regenerated per lane;
 *   2. the AFS-64 lane operation is inlined and the split rounds are
 *      selected from the instance profile;
 *   3. when compiled with -DAFS_TREDM_USE_AVX2 -mavx2, the AFS-64
 *      nonlinear layer processes four 64-bit lanes at a time with AVX2 while
 *      keeping the audited opt64 AFS-LMDS-1600-S6 linear layer.
 *
 * The bit-level behavior is identical to the reference implementation.
 */
#include "afs_p1600.h"
#include "../common/s6/afs_lmds1600_s6_opt64.h"

#include <stdint.h>

#if defined(AFS_TREDM_USE_AVX2) && defined(__AVX2__)
#include <immintrin.h>
#define AFS_TREDM_HAVE_AVX2 1
#else
#define AFS_TREDM_HAVE_AVX2 0
#endif

/* === AFS-TrEDM Round-2 AVX2 patch: skip the Round-1 g/h/permute definitions when the Round-2 AVX2 fast path provides them. === */
#if !defined(AFS_TREDM_USE_AVX2_R2)
/* Function rotl64: rotates a 64-bit word left by a constant-count modulo 64 amount. */
static inline uint64_t rotl64(uint64_t x, unsigned n)
{
    n &= 63U;
    return (uint64_t)((x << n) | (x >> ((64U - n) & 63U)));
}

/* Function rotr32: rotates a 32-bit word right by a constant-count modulo 32 amount. */
static inline uint32_t rotr32(uint32_t x, unsigned n)
{
    n &= 31U;
    return (uint32_t)((x >> n) | (x << ((32U - n) & 31U)));
}

#if (defined(__x86_64__) || defined(__i386__)) && \
    (defined(__GNUC__) || defined(__clang__)) && \
    defined(AFS_TREDM_USE_X86_SBOX_ASM)
#define AFS_TREDM_HAVE_X86_SBOX_ASM 1
/* Function afs64_t5_k2_x86_asm_inline: computes the inlined optional x86 assembly AFS64_t5_k2 S-box. */
static inline uint64_t afs64_t5_k2_x86_asm_inline(uint64_t in, uint32_t c)
{
    uint32_t x = (uint32_t)(in >> 32);
    uint32_t y = (uint32_t)in;
    uint32_t t;

    __asm__ __volatile__(
        "movl %[y], %[t]\n\t"
        "rorl $17, %[t]\n\t"
        "addl %[t], %[x]\n\t"
        "xorl %[c], %[x]\n\t"
        "movl %[x], %[t]\n\t"
        "rorl $24, %[t]\n\t"
        "addl %[t], %[y]\n\t"
        "xorl %[c], %[y]\n\t"
        "movl %[y], %[t]\n\t"
        "rorl $1, %[t]\n\t"
        "xorl %[t], %[x]\n\t"
        "movl %[x], %[t]\n\t"
        "rorl $1, %[t]\n\t"
        "xorl %[t], %[y]\n\t"
        "movl %[y], %[t]\n\t"
        "rorl $16, %[t]\n\t"
        "xorl %[t], %[x]\n\t"
        "movl %[x], %[t]\n\t"
        "rorl $31, %[t]\n\t"
        "xorl %[t], %[y]\n\t"
        "movl %[y], %[t]\n\t"
        "rorl $24, %[t]\n\t"
        "addl %[t], %[x]\n\t"
        "xorl %[c], %[x]\n\t"
        "addl %[x], %[y]\n\t"
        "xorl %[c], %[y]\n\t"
        : [x] "+&r"(x), [y] "+&r"(y), [t] "=&r"(t)
        : [c] "r"(c)
        : "cc");

    return (((uint64_t)x) << 32) | (uint64_t)y;
}
#endif

/* Function afs64_t5_k2_inline: computes the inlined scalar AFS64_t5_k2 ARX S-box. */
static inline uint64_t afs64_t5_k2_inline(uint64_t in, uint32_t c)
{
#if defined(AFS_TREDM_HAVE_X86_SBOX_ASM)
    return afs64_t5_k2_x86_asm_inline(in, c);
#else
    uint32_t x = (uint32_t)(in >> 32);
    uint32_t y = (uint32_t)in;

    /* AFS-64, A8=11000011, K8=[17,24,1,1,16,31,24,0]. */
    x = (uint32_t)(x + rotr32(y, 17)); x ^= c;
    y = (uint32_t)(y + rotr32(x, 24)); y ^= c;
    x ^= rotr32(y, 1);
    y ^= rotr32(x, 1);
    x ^= rotr32(y, 16);
    y ^= rotr32(x, 31);
    x = (uint32_t)(x + rotr32(y, 24)); x ^= c;
    y = (uint32_t)(y + rotr32(x, 0));  y ^= c;

    return (((uint64_t)x) << 32) | (uint64_t)y;
#endif
}

static const uint32_t AFS_RC[24][25] = {
    {0x1273A74AU, 0xE4BF7132U, 0x1BEE1902U, 0x6536539FU, 0x32F8B9E0U, 0x6A54944AU, 0x19F2B49DU, 0xB06D50CCU, 0x3026A473U, 0x807952CAU, 0x892CDA9BU, 0xEF31A906U, 0x0EBD762DU, 0x6C72BDC1U, 0x15DFF62FU, 0x1F226260U, 0xD629819BU, 0xBBE5C976U, 0xF788C9C0U, 0x2A26F738U, 0x23C026EEU, 0x5AE88BDFU, 0x10E4831DU, 0x159B977AU, 0xAAAF89B6U}, /* round 0 */
    {0x131FF2B0U, 0xB3240503U, 0x6D15EC7FU, 0x28908BF6U, 0x9DB9BAA9U, 0x9C38FF11U, 0x31597D9BU, 0x03345313U, 0x841166A5U, 0x13C92CC4U, 0xA0096628U, 0x02B4116AU, 0xBC5F6449U, 0xD7A8089BU, 0x336B5E19U, 0x55A9DCF7U, 0x79643EDEU, 0x09111FC5U, 0x224CC140U, 0x1EEFCDE9U, 0x68F67EE5U, 0x283741A2U, 0x5D26B4A1U, 0x4F9320FDU, 0x6179F0BCU}, /* round 1 */
    {0x4CBA855EU, 0xFD10EE92U, 0x945AE5FBU, 0xAD557A48U, 0x5DB51FA2U, 0xE962BC43U, 0x5A30ABA5U, 0x61523BD6U, 0xB518D666U, 0x7E576F31U, 0x66FD8F27U, 0x770AFA44U, 0xF83DD10DU, 0xBF13F78BU, 0x54938DDBU, 0xADE0C7ADU, 0xC3F16EAFU, 0x5077E9CDU, 0x46934BA1U, 0xB48CF4C0U, 0x29E35198U, 0x7FA2FFA3U, 0x297BEA95U, 0xD1C49541U, 0xCE54B32EU}, /* round 2 */
    {0xF086B177U, 0xE044048DU, 0x7A25EE7DU, 0x6B16C965U, 0xB181A930U, 0x665BA10BU, 0xFBB9F2CAU, 0x88A56076U, 0x1FED3280U, 0xA57EDA4FU, 0xA3B633FDU, 0x8D641A3FU, 0xA6F8EAF0U, 0x37749639U, 0xF1D913C1U, 0x8222270FU, 0xD83D1C4AU, 0xCCC1CED8U, 0xC3739655U, 0xF78DDE11U, 0xF133D580U, 0xD59F1EFFU, 0x30E157E4U, 0x82D55481U, 0xCD1C18C7U}, /* round 3 */
    {0x87A141F7U, 0xC275D5AFU, 0x35479AA4U, 0xF392ACA4U, 0x4A7B3DC2U, 0xD8483A71U, 0x1A9122CDU, 0x810F9C94U, 0xB7F09EE9U, 0xC49025EBU, 0x4B199041U, 0xF78202B7U, 0x938C4610U, 0x639B86F5U, 0xE34DAD27U, 0x835ED9E3U, 0x75B2CD73U, 0x8B7C149FU, 0x54410FD6U, 0xEBB95F6CU, 0x79D3D5E9U, 0x47497D07U, 0x4B26AF82U, 0x001EF9A2U, 0xD45BA699U}, /* round 4 */
    {0xCB806EABU, 0xD55FE990U, 0x22E8AE85U, 0x27BE327BU, 0xAFEEB8CAU, 0x7F338625U, 0x606E9463U, 0x5644DF4EU, 0x6CD6901FU, 0x809AA39BU, 0x5E715448U, 0xCDC382E4U, 0x02003D21U, 0x6D75220EU, 0xEB0142F1U, 0xD70B2E22U, 0xBEDF2F61U, 0x1C884F6FU, 0xC89BC075U, 0x5D46ED39U, 0x46EFE346U, 0xB750D123U, 0xE4292233U, 0x97C634C8U, 0xE06418D3U}, /* round 5 */
    {0xC89035A0U, 0x377F504DU, 0x03D06410U, 0xE73EBBDFU, 0xB2345B6AU, 0x54D8705EU, 0xEB0015AFU, 0x7A100DB0U, 0x500397B1U, 0x7BC419B4U, 0x3E626DFAU, 0xE5279FF7U, 0x972C5244U, 0x8A8C062AU, 0x762C730FU, 0xABDDE73CU, 0x9803A313U, 0x7FDAE151U, 0x6AE20752U, 0x601E92F2U, 0x4B682400U, 0x0ED082F4U, 0xA14ADB1BU, 0x8788C1A3U, 0x5E08ADB8U}, /* round 6 */
    {0x844D5C28U, 0x6CE3BCA4U, 0x26DFAB6CU, 0xB27BE1CCU, 0x6A342215U, 0xEF681D6EU, 0x6FEF8C9DU, 0x4A5DF33CU, 0xAF645CCBU, 0x6147CA80U, 0xA0B8D1D1U, 0xA151F87DU, 0x2EFA6BE5U, 0xE84883ECU, 0x1B7E77FFU, 0x353EE8ABU, 0xA208DAB6U, 0x9DBE822CU, 0x31889E4DU, 0xA9D1E8D4U, 0x122E0B1CU, 0x609D0298U, 0xC0694A21U, 0x3ACF84E1U, 0x0F66BA47U}, /* round 7 */
    {0x50F8D888U, 0x640E4664U, 0xE1A2865CU, 0xA25C7FF7U, 0x7FE67263U, 0x151C087FU, 0x44C87484U, 0x0C2E1F78U, 0xEF045BC7U, 0xD5C9FF98U, 0xCD365B86U, 0xA71FCEF1U, 0x2E8B50D1U, 0x955ABC7CU, 0xB65D3B23U, 0xB705FC74U, 0xA4DC4541U, 0x93BADE19U, 0xEE60AB81U, 0xE256DDB0U, 0x72A5FD60U, 0x8BB98892U, 0x609C0F4BU, 0x0A4CB674U, 0xCD61B3A8U}, /* round 8 */
    {0x6C847EC8U, 0x16235031U, 0x9CAC1E76U, 0xFF3F5DF1U, 0x3FB13771U, 0xF0701357U, 0xA88EECF5U, 0x6267292CU, 0x0FFDA352U, 0x30D24EFEU, 0xA8F46D6CU, 0x6554D1DFU, 0xF152AF2EU, 0x5AC07D8FU, 0x2AD0915EU, 0x29669E82U, 0x13C99617U, 0xFD724852U, 0x50F87A3EU, 0xBC7B1EA3U, 0xEB2F16B4U, 0xF0E9DE62U, 0xB56DAEF1U, 0x3557B73DU, 0x05A44220U}, /* round 9 */
    {0x0CA1A4A0U, 0x8C157376U, 0x65C124ECU, 0x509DE35AU, 0x0E2BFBA4U, 0xE7F83BFDU, 0x9B90BAC0U, 0x5B2F7D2AU, 0x9F94D1AFU, 0x18246718U, 0x879321B2U, 0x6380CDEBU, 0x6D01E183U, 0xF61322A1U, 0xE0CEE078U, 0x6C93C26AU, 0x368E9A7CU, 0xFB160F82U, 0x0CB1FCD8U, 0x10762A55U, 0x54D5A780U, 0x5266DE5CU, 0x30D1D119U, 0xF15FF917U, 0xFAD7B291U}, /* round 10 */
    {0xFA07A0D6U, 0x7E7327CEU, 0x729E9A1DU, 0x746E2A04U, 0x59217C03U, 0xB128FC80U, 0x1D9CF3B8U, 0x8B037BC6U, 0x06881DFAU, 0x90EEABF7U, 0x57017226U, 0xAD016FA9U, 0xDA4C2F64U, 0xBFCCD61BU, 0x24B68AC7U, 0x7BFA89F7U, 0xCCFCE667U, 0xEF444CFDU, 0xDCF6E635U, 0xB30357A8U, 0xE5559498U, 0xFA97502CU, 0xBA970481U, 0xD3A90904U, 0x05BEE1FBU}, /* round 11 */
    {0xAB87DAD8U, 0xEC03D583U, 0x0B7AE807U, 0x1DA34F04U, 0x3D8CC306U, 0x44A2D61DU, 0x2FACE8C0U, 0x33D5F256U, 0x69DBD50EU, 0x6A285414U, 0xD4DB2C31U, 0x1EE45720U, 0x1CE306BDU, 0x3F5F7792U, 0x313FF4DCU, 0x6349BE2EU, 0xD48E38C5U, 0x5BCCB58BU, 0x66C1018CU, 0x56F446AEU, 0x26A51805U, 0x91C5446EU, 0x29E11B86U, 0x66A76578U, 0x04A082C8U}, /* round 12 */
    {0xCD4706BBU, 0xF9B27532U, 0x4580B7ECU, 0xA33CD664U, 0xF7A8FAB9U, 0x12E3B79CU, 0x2ED2A5A1U, 0xEBD4E44EU, 0x65F63DD6U, 0xC696C4F5U, 0x2DF12315U, 0xD65CA4A8U, 0x80D54A8FU, 0x376753E6U, 0xB3C6E57BU, 0x14BDC79FU, 0xC03A66AFU, 0x5A3799B3U, 0x5E9719CAU, 0x61647B87U, 0x925184DFU, 0xAC54C6A9U, 0x0019B64DU, 0x6A79AF6EU, 0xB372978DU}, /* round 13 */
    {0x8F509DE6U, 0x659C3852U, 0x4ABD00D5U, 0xA224E3DEU, 0xA3DF11B4U, 0xCCBACD04U, 0xE2C0A037U, 0x104B5F51U, 0x8564D829U, 0x1CA1D27FU, 0xAFEA165FU, 0x004DA5C9U, 0x2283F7FAU, 0x71A882AAU, 0x71DBA9E4U, 0x15728FAAU, 0x01BAC85CU, 0xC81D5334U, 0x0002E649U, 0x29DA9221U, 0x7399B453U, 0xC9F15514U, 0x9F7E4167U, 0x4849B357U, 0x50E906DAU}, /* round 14 */
    {0xCB9587CAU, 0xCC76F111U, 0x24B93B8AU, 0xAC4081BEU, 0xC516A6E7U, 0xA8E5034EU, 0x373A3FB4U, 0x761AE09CU, 0xC5F5B24AU, 0x0272E9B4U, 0xE144B61FU, 0xA9CB43A0U, 0x87311A37U, 0x6AD0DCF7U, 0x66EEC51DU, 0x4DCCDCDFU, 0xC87FD8C8U, 0xD7616439U, 0xC1E72AFCU, 0xBA523DE4U, 0x250E961FU, 0x036CB7B1U, 0xD3D334FCU, 0xA527D44FU, 0xC9CFF453U}, /* round 15 */
    {0x8160766CU, 0x2C3C9FC7U, 0xBF4B76DBU, 0x976D1EB4U, 0x094ECF45U, 0x6B8D6DC4U, 0xC370EF0CU, 0x6F1018E8U, 0xFDDA3ECAU, 0xF29A74D5U, 0x59830BC8U, 0x1F8249F7U, 0x2AB34DECU, 0x73974127U, 0x4638F7CAU, 0xF7E2E7AFU, 0x5E8E1B9BU, 0x9D9947BDU, 0x2A7FEC7AU, 0xA4D336BDU, 0xAD102195U, 0x818E421EU, 0x04B52015U, 0xE57C6FC3U, 0xEDE0DADEU}, /* round 16 */
    {0x705C3220U, 0xD211274AU, 0xAF4E1B84U, 0xAF8713A4U, 0xE047D3FEU, 0xBF7F7140U, 0xCAB3C499U, 0x90D95D55U, 0x33EC63A0U, 0xCB42C39FU, 0xA8DB4477U, 0xC90D736DU, 0xE2BDCFFDU, 0x431CB17DU, 0xF8EFD150U, 0x0019E5C2U, 0xE1143B87U, 0x97CD62C8U, 0xEF7B5192U, 0x3E855F42U, 0x3426F893U, 0xBF299C70U, 0x45B21453U, 0x4BEB12A5U, 0x72973A0FU}, /* round 17 */
    {0x2035B566U, 0x6D0561EAU, 0x3A6E583AU, 0x778691F5U, 0x886640A9U, 0xC04E65CBU, 0xA1EB1189U, 0x00990BB5U, 0x8835AB13U, 0x39B9DB66U, 0xFCFC70CEU, 0x24722895U, 0xD65C4420U, 0xC5300624U, 0x99D82825U, 0x9E066310U, 0x675FB499U, 0x5AA8BCF6U, 0x5BD9CB8FU, 0x9B1514EBU, 0x439A98E7U, 0x1DBFDD10U, 0x7264CE72U, 0x5BF0C43EU, 0x8FEE684AU}, /* round 18 */
    {0x20BE8415U, 0x48BB9BEEU, 0x511AFF4EU, 0x0BE2FC33U, 0x111AC62FU, 0xC5AB67A5U, 0xE92AAA7CU, 0x895BBF21U, 0xF3282737U, 0x441DEFBDU, 0xFBA0D59AU, 0xA2D48EB3U, 0xD84E266CU, 0x6176D79EU, 0x223DC3DEU, 0x74B094C7U, 0xCB10B8FEU, 0x255B9B9CU, 0x7C5CC3BAU, 0xDC8FCC3DU, 0xC33860C1U, 0xEC0659FCU, 0xAD34E9ECU, 0x5C1B4395U, 0xB1DFD515U}, /* round 19 */
    {0x9D280585U, 0x3775150FU, 0xD4230BFEU, 0xD7D6A2F6U, 0x84178726U, 0x0BA89E22U, 0x6871905AU, 0xF1FE1ABAU, 0x7392EFEAU, 0x115A42D7U, 0x06ADCDFDU, 0x036216B6U, 0xB0FBBC7FU, 0x88333F83U, 0x83DB5F0EU, 0xA4365C2CU, 0xE0A6DCA8U, 0x1458C9E6U, 0xFDAA4840U, 0x909A8244U, 0xE66CCD0AU, 0xC09EEFF1U, 0xBD0C2A49U, 0x9A63471BU, 0x3F4E136AU}, /* round 20 */
    {0x226550C9U, 0xDF8AC353U, 0xAD38672EU, 0xDAEDBF1AU, 0x6C3693BDU, 0x1F74D25CU, 0x394C3AF5U, 0x2FF03F88U, 0x5D3C4E56U, 0xF02266BAU, 0xA21A21CFU, 0xD033659BU, 0x8D4CB587U, 0xB608877BU, 0x694346ECU, 0x2F190A0FU, 0xC641D53DU, 0x2A6D7D4AU, 0xBA1209F4U, 0x28ADF498U, 0x84C0114EU, 0xBB018DA5U, 0x2BABDA67U, 0xE710B400U, 0xEB0E58C0U}, /* round 21 */
    {0xAFC885A9U, 0xBACDCC31U, 0x912AE9AFU, 0x3CBD42BDU, 0x8E56AA6AU, 0x3FB162B8U, 0x0CACE01EU, 0x43DC75F6U, 0x0BDE83B7U, 0xB8E78BF7U, 0x4963AAB0U, 0x16A17E69U, 0x61361E7DU, 0x3527EBEBU, 0xF8E96A7BU, 0x7D89C2B2U, 0xCA80271BU, 0x3A4E1F51U, 0x50F10013U, 0x5A607A00U, 0x159DEAD0U, 0x929E703BU, 0x2B26A71FU, 0x18958C03U, 0xC6A0C0F1U}, /* round 22 */
    {0x02F79EF5U, 0x62562B78U, 0xBBC8869EU, 0x96268CC6U, 0x751C26D1U, 0x019D9345U, 0xAC0EE378U, 0xBF5BCA56U, 0x35D7CEBCU, 0xEEEF94F2U, 0x811B7FBBU, 0x64EA046AU, 0x4D5B1AB1U, 0x00A472CBU, 0xFC60665EU, 0x149D4B85U, 0x127BD662U, 0x9F2B4F28U, 0xFC43B805U, 0xDD2217E4U, 0xFAB1777DU, 0xC5EA2864U, 0x3D3225F7U, 0x35216FF1U, 0x2FA8DF4FU} /* round 23 */
};

#if AFS_TREDM_HAVE_AVX2
/* Function afs_rotr32x8: implements the indicated AFS-TrEDM helper routine. */
static inline __m256i afs_rotr32x8(__m256i v, int n)
{
    return _mm256_or_si256(_mm256_srli_epi32(v, n),
                           _mm256_slli_epi32(v, 32 - n));
}

/* Function afs_rotr32x8_16: implements the indicated AFS-TrEDM helper routine. */
static inline __m256i afs_rotr32x8_16(__m256i v)
{
    const __m256i shuf = _mm256_setr_epi8(
        2, 3, 0, 1,  6, 7, 4, 5,  10, 11, 8, 9,  14, 15, 12, 13,
        2, 3, 0, 1,  6, 7, 4, 5,  10, 11, 8, 9,  14, 15, 12, 13);
    return _mm256_shuffle_epi8(v, shuf);
}

/* Function afs_rotr32x8_24: implements the indicated AFS-TrEDM helper routine. */
static inline __m256i afs_rotr32x8_24(__m256i v)
{
    const __m256i shuf = _mm256_setr_epi8(
        3, 0, 1, 2,  7, 4, 5, 6,  11, 8, 9, 10,  15, 12, 13, 14,
        3, 0, 1, 2,  7, 4, 5, 6,  11, 8, 9, 10,  15, 12, 13, 14);
    return _mm256_shuffle_epi8(v, shuf);
}

/* Function afs64x8_t5_k2_avx2: computes the AVX2 vectorized AFS64_t5_k2 S-box over packed lanes. */
static inline void afs64x8_t5_k2_avx2(__m256i *x_io, __m256i *y_io, __m256i c)
{
    __m256i x = *x_io;
    __m256i y = *y_io;

    x = _mm256_add_epi32(x, afs_rotr32x8(y, 17)); x = _mm256_xor_si256(x, c);
    y = _mm256_add_epi32(y, afs_rotr32x8_24(x)); y = _mm256_xor_si256(y, c);
    x = _mm256_xor_si256(x, afs_rotr32x8(y, 1));
    y = _mm256_xor_si256(y, afs_rotr32x8(x, 1));
    x = _mm256_xor_si256(x, afs_rotr32x8_16(y));
    y = _mm256_xor_si256(y, afs_rotr32x8(x, 31));
    x = _mm256_add_epi32(x, afs_rotr32x8_24(y)); x = _mm256_xor_si256(x, c);
    y = _mm256_add_epi32(y, x); y = _mm256_xor_si256(y, c);

    *x_io = x;
    *y_io = y;
}

/* Function afs_load_split8: implements the indicated AFS-TrEDM helper routine. */
static inline void afs_load_split8(const uint64_t *a, __m256i *x_out, __m256i *y_out)
{
    __m256i v0 = _mm256_loadu_si256((const __m256i *)(const void *)(a + 0));
    __m256i v1 = _mm256_loadu_si256((const __m256i *)(const void *)(a + 4));
    __m256i s0 = _mm256_shuffle_epi32(v0, 0xD8);
    __m256i s1 = _mm256_shuffle_epi32(v1, 0xD8);
    __m256i p0 = _mm256_permute4x64_epi64(s0, 0xD8);
    __m256i p1 = _mm256_permute4x64_epi64(s1, 0xD8);

    *y_out = _mm256_permute2x128_si256(p0, p1, 0x20);
    *x_out = _mm256_permute2x128_si256(p0, p1, 0x31);
}

/* Function afs_store_split8: implements the indicated AFS-TrEDM helper routine. */
static inline void afs_store_split8(uint64_t *a, __m256i x, __m256i y)
{
    __m256i lo = _mm256_unpacklo_epi32(y, x);
    __m256i hi = _mm256_unpackhi_epi32(y, x);
    __m256i v0 = _mm256_permute2x128_si256(lo, hi, 0x20);
    __m256i v1 = _mm256_permute2x128_si256(lo, hi, 0x31);

    _mm256_storeu_si256((__m256i *)(void *)(a + 0), v0);
    _mm256_storeu_si256((__m256i *)(void *)(a + 4), v1);
}

/* Function afs_p1600_nonlinear_avx2: applies the AVX2 nonlinear AFS-64 layer to groups of lanes. */
static inline void afs_p1600_nonlinear_avx2(uint64_t A[AFS_P1600_LANES], const uint32_t rc[25])
{
    __m256i x0, y0, x1, y1, x2, y2;
    __m256i c0, c1, c2;

    afs_load_split8(A + 0,  &x0, &y0);
    afs_load_split8(A + 8,  &x1, &y1);
    afs_load_split8(A + 16, &x2, &y2);

    c0 = _mm256_loadu_si256((const __m256i *)(const void *)(rc + 0));
    c1 = _mm256_loadu_si256((const __m256i *)(const void *)(rc + 8));
    c2 = _mm256_loadu_si256((const __m256i *)(const void *)(rc + 16));

    afs64x8_t5_k2_avx2(&x0, &y0, c0);
    afs64x8_t5_k2_avx2(&x1, &y1, c1);
    afs64x8_t5_k2_avx2(&x2, &y2, c2);

    afs_store_split8(A + 0,  x0, y0);
    afs_store_split8(A + 8,  x1, y1);
    afs_store_split8(A + 16, x2, y2);

    A[24] = afs64_t5_k2_inline(A[24], rc[24]);
}
#endif

/* Function afs_p1600_nonlinear: applies the scalar optimized nonlinear AFS-64 layer to all lanes. */
static inline void afs_p1600_nonlinear(uint64_t A[AFS_P1600_LANES], const uint32_t rc[25])
{
#if AFS_TREDM_HAVE_AVX2
    afs_p1600_nonlinear_avx2(A, rc);
#else
    unsigned i;
    for (i = 0; i < 25U; i++) {
        A[i] = afs64_t5_k2_inline(A[i], rc[i]);
    }
#endif
}

/* Function afs_p1600_round_opt: applies one optimized AFS-p-S6 permutation round to the 1600-bit state. */
static inline void afs_p1600_round_opt(uint64_t A[AFS_P1600_LANES], unsigned round, const uint32_t rc[25])
{
    afs_p1600_nonlinear(A, rc);

    afs_lmds1600_s6_opt64(A, round);
}

/* Function afs_p1600_permute: applies a consecutive range of AFS-p-S6 rounds. */
void afs_p1600_permute(uint64_t A[AFS_P1600_LANES], unsigned first_round, unsigned nr)
{
    unsigned i;
    for (i = 0; i < nr; i++) {
        afs_p1600_round_opt(A, first_round + i, AFS_RC[first_round + i]);
    }
}

/* Function afs_p1600_g: applies the front half g of the split permutation. */
void afs_p1600_g(uint64_t A[AFS_P1600_LANES])
{
#if AFS_P1600_SPLIT_ROUNDS == 12
    afs_p1600_round_opt(A, 0U, AFS_RC[0]);
    afs_p1600_round_opt(A, 1U, AFS_RC[1]);
    afs_p1600_round_opt(A, 2U, AFS_RC[2]);
    afs_p1600_round_opt(A, 3U, AFS_RC[3]);
    afs_p1600_round_opt(A, 4U, AFS_RC[4]);
    afs_p1600_round_opt(A, 5U, AFS_RC[5]);
    afs_p1600_round_opt(A, 6U, AFS_RC[6]);
    afs_p1600_round_opt(A, 7U, AFS_RC[7]);
    afs_p1600_round_opt(A, 8U, AFS_RC[8]);
    afs_p1600_round_opt(A, 9U, AFS_RC[9]);
    afs_p1600_round_opt(A, 10U, AFS_RC[10]);
    afs_p1600_round_opt(A, 11U, AFS_RC[11]);
#else
    afs_p1600_permute(A, 0U, AFS_P1600_SPLIT_ROUNDS);
#endif
}

/* Function afs_p1600_h: applies the back half h of the split permutation. */
void afs_p1600_h(uint64_t A[AFS_P1600_LANES])
{
#if AFS_P1600_SPLIT_ROUNDS == 12
    afs_p1600_round_opt(A, 12U, AFS_RC[12]);
    afs_p1600_round_opt(A, 13U, AFS_RC[13]);
    afs_p1600_round_opt(A, 14U, AFS_RC[14]);
    afs_p1600_round_opt(A, 15U, AFS_RC[15]);
    afs_p1600_round_opt(A, 16U, AFS_RC[16]);
    afs_p1600_round_opt(A, 17U, AFS_RC[17]);
    afs_p1600_round_opt(A, 18U, AFS_RC[18]);
    afs_p1600_round_opt(A, 19U, AFS_RC[19]);
    afs_p1600_round_opt(A, 20U, AFS_RC[20]);
    afs_p1600_round_opt(A, 21U, AFS_RC[21]);
    afs_p1600_round_opt(A, 22U, AFS_RC[22]);
    afs_p1600_round_opt(A, 23U, AFS_RC[23]);
#else
    afs_p1600_permute(A, AFS_P1600_SPLIT_ROUNDS, AFS_P1600_SPLIT_ROUNDS);
#endif
}
#endif /* !AFS_TREDM_USE_AVX2_R2 */
/* === end AFS-TrEDM Round-2 AVX2 patch === */
