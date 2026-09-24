#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "hash_garnet.h"
#include "CryptHash_Garnet.h"
#include "aesT.h"

// 保持原有的类型定义
#ifndef custom_types_defined
#define custom_types_defined
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#endif
// 定义状态大小
#define STATE_5x5 25
#define STATE_4x4 16

#define SHL64(x, r) x << r
#define SHR64(x, r) x >> r
// Counter 解析引擎宏
#define GET_MODE(c)       ((c) >> 47     )               // bit63: 0=Sponge, 1=Sponge-DM
#define GET_RATE_BITS(c)  (((c) >> 32) & 0x7FFF)          // 速率(比特)
#define GET_INIT_RNDS(c)  (((c) >> 24) & 0xFF)            // 初始化阶段轮数
#define GET_ABS_RNDS(c)   (((c) >> 16) & 0xFF)            // 吸收阶段轮数
#define GET_MID_RNDS(c)   (((c) >> 8) & 0xFF)            // 中间处理阶段轮数
#define GET_SQZ_RNDS(c)   (((c) >> 0) & 0xFF)            // 挤压阶段轮数
#define IS_5X5_MATRIX(c)  ((c) == COUNTER_1024_W1024 || (c) == COUNTER_1024_W1152 || \
                           (c) == COUNTER_1024_W2048_DM)
#define IS_FLAGSHIP_5x5(c) ((c) == COUNTER_1024_W2048_DM)
// 4x4 DM 判定
#define IS_4x4_DM(c) (((c) & COUNTER_1024A_SP_DM) && !IS_5X5_MATRIX(c))


// 常量定义（π的前512位）
/*
static const uint128_t C0_n = { .u = { .v = { 0xC4C6628BULL << 32 | 0x80DC1CD1ULL, 0xC90FDAA2ULL << 32 | 0x2168C234ULL } } };
static const uint128_t C1_n = { .u = { .v = { 0x020BBEA6ULL << 32 | 0x3B139B22ULL, 0x29024E08ULL << 32 | 0x8A67CC74ULL } } };
static const uint128_t C2_n = { .u = { .v = { 0xEF9519B3ULL << 32 | 0xCD3A431BULL, 0x514A0879ULL << 32 | 0x8E3404DDULL } } };
static const uint128_t C3_n = { .u = { .v = { 0x4FE1356DULL << 32 | 0x6D51C245ULL, 0x302B0A6DULL << 32 | 0xF25F1437ULL } } };
static const uint128_t C4_n = { .u = { .v = { 0xF44C42E9ULL << 32 | 0xA637ED6BULL, 0xE485B576ULL << 32 | 0x625E7EC6ULL } } };
*/

// 斐波那契常数（4x4状态）
static const uint64_t Fibonacci_4x4[16][3] GARNET_FAST_TABLE = 
{
    {1, 1, 2}, {3, 5, 8}, {13, 21, 34}, {55, 89, 144},
    {233, 377, 610}, {987, 1597, 2584}, {4181, 6765, 10946},
    {17711, 28657, 46368}, {75025, 121393, 196418},
    {317811, 514229, 832040}, {1346269, 2178309, 3524578},
    {5702887, 9227465, 14930352}, {24157817, 39088169, 63245986},
    {102334155, 165580141, 267914296}, {433494437, 701408733, 1134903170},
    {1836311903, 2971215073, 4807526976}
};
// 斐波那契常数（5x5状态）
static const uint64_t Fibonacci_5x5[16][4] GARNET_FAST_TABLE =
{
    {1, 1, 2, 3}, {5, 8, 13, 21}, {34, 55, 89, 144}, {233, 377, 610, 987},
    {1597, 2584, 4181, 6765}, {10946, 17711, 28657, 46368},
    {75025, 121393, 196418, 317811}, {514229, 832040, 1346269, 2178309},
    {3524578, 5702887, 9227465, 14930352}, {24157817, 39088169, 63245986, 102334155},
    {165580141, 267914296, 433494437, 701408733}, {1134903170, 1836311903, 2971215073, 4807526976},
    {7778742049, 12586269025, 20365011074, 32951280099}, {53316291173, 86267571272, 139583862445, 225851433717},
    {365435296162, 591286729879, 956722026041, 1548008755920}, {2504730781961, 4052739537881, 6557470319842, 10610209857723}
};




static const uint64_t reduction_table_1_MC4[2] GARNET_FAST_TABLE =
{
    0x0, 0x87,
};//对应于4阶矩阵的Mixcolumn中的x^128+x^7+x^2+x+1乘以\alpha


//-------------------------

static const uint64_t reduction_table_1_MD4[2] GARNET_FAST_TABLE =
{
    0x0000000000000000,0x0000000020008005, 
};//对应于4阶矩阵的Mixdiagonal中的x^128+x^29+x^15+x^2+1乘以\alpha



static const uint64_t reduction_table_1_MC5[2] GARNET_FAST_TABLE =
{
    0x0, 0x0000008010000005,
}; //对应于5阶矩阵的mixcolun中的x^128 + x^39+ x^28 + x^2 + 1乘以\alpha

static const uint64_t reduction_table_2_MC5[4] GARNET_FAST_TABLE =
{
0x0000000000000000,0x0000008010000005,0x000001002000000a, 0x000001803000000f,
};//对应于5阶矩阵的mixcolun中的x^128 + x^39+ x^28 + x^2 + 1乘以\alpha^2

static const uint64_t reduction_table_4_MC5[16] GARNET_FAST_TABLE =
{
    0x0000000000000000,0x0000008010000005,0x000001002000000a,0x000001803000000f,0x0000020040000014,0x0000028050000011,0x000003006000001e,0x000003807000001b,
    0x0000040080000028,0x000004809000002d,0x00000500a0000022,0x00000580b0000027,0x00000600c000003c,0x00000680d0000039,0x00000700e0000036,0x00000780f0000033,
};//对应于5阶矩阵的mixcolun中的x^128 + x^39+ x^28 + x^2 + 1乘以\alpha^4


static const uint64_t reduction_table_8_MC5[256] GARNET_FAST_TABLE =
{
   0x0000000000000000,0x0000008010000005,0x000001002000000a,0x000001803000000f,0x0000020040000014,0x0000028050000011,0x000003006000001e,0x000003807000001b,0x0000040080000028,0x000004809000002d,0x00000500a0000022,0x00000580b0000027,0x00000600c000003c,0x00000680d0000039,0x00000700e0000036,0x00000780f0000033,
   0x0000080100000050,0x0000088110000055,0x000009012000005a,0x000009813000005f,0x00000a0140000044,0x00000a8150000041,0x00000b016000004e,0x00000b817000004b,0x00000c0180000078,0x00000c819000007d,0x00000d01a0000072,0x00000d81b0000077,0x00000e01c000006c,0x00000e81d0000069,0x00000f01e0000066,0x00000f81f0000063,
   0x00001002000000a0,0x00001082100000a5,0x00001102200000aa,0x00001182300000af,0x00001202400000b4,0x00001282500000b1,0x00001302600000be,0x00001382700000bb,0x0000140280000088,0x000014829000008d,0x00001502a0000082,0x00001582b0000087,0x00001602c000009c,0x00001682d0000099,0x00001702e0000096,0x00001782f0000093,
   0x00001803000000f0,0x00001883100000f5,0x00001903200000fa,0x00001983300000ff,0x00001a03400000e4,0x00001a83500000e1,0x00001b03600000ee,0x00001b83700000eb,0x00001c03800000d8,0x00001c83900000dd,0x00001d03a00000d2,0x00001d83b00000d7,0x00001e03c00000cc,0x00001e83d00000c9,0x00001f03e00000c6,0x00001f83f00000c3,
   0x0000200400000140,0x0000208410000145,0x000021042000014a,0x000021843000014f,0x0000220440000154,0x0000228450000151,0x000023046000015e,0x000023847000015b,0x0000240480000168,0x000024849000016d,0x00002504a0000162,0x00002584b0000167,0x00002604c000017c,0x00002684d0000179,0x00002704e0000176,0x00002784f0000173,
   0x0000280500000110,0x0000288510000115,0x000029052000011a,0x000029853000011f,0x00002a0540000104,0x00002a8550000101,0x00002b056000010e,0x00002b857000010b,0x00002c0580000138,0x00002c859000013d,0x00002d05a0000132,0x00002d85b0000137,0x00002e05c000012c,0x00002e85d0000129,0x00002f05e0000126,0x00002f85f0000123,
   0x00003006000001e0,0x00003086100001e5,0x00003106200001ea,0x00003186300001ef,0x00003206400001f4,0x00003286500001f1,0x00003306600001fe,0x00003386700001fb,0x00003406800001c8,0x00003486900001cd,0x00003506a00001c2,0x00003586b00001c7,0x00003606c00001dc,0x00003686d00001d9,0x00003706e00001d6,0x00003786f00001d3,
   0x00003807000001b0,0x00003887100001b5,0x00003907200001ba,0x00003987300001bf,0x00003a07400001a4,0x00003a87500001a1,0x00003b07600001ae,0x00003b87700001ab,0x00003c0780000198,0x00003c879000019d,0x00003d07a0000192,0x00003d87b0000197,0x00003e07c000018c,0x00003e87d0000189,0x00003f07e0000186,0x00003f87f0000183,
   0x0000400800000280,0x0000408810000285,0x000041082000028a,0x000041883000028f,0x0000420840000294,0x0000428850000291,0x000043086000029e,0x000043887000029b,0x00004408800002a8,0x00004488900002ad,0x00004508a00002a2,0x00004588b00002a7,0x00004608c00002bc,0x00004688d00002b9,0x00004708e00002b6,0x00004788f00002b3,
   0x00004809000002d0,0x00004889100002d5,0x00004909200002da,0x00004989300002df,0x00004a09400002c4,0x00004a89500002c1,0x00004b09600002ce,0x00004b89700002cb,0x00004c09800002f8,0x00004c89900002fd,0x00004d09a00002f2,0x00004d89b00002f7,0x00004e09c00002ec,0x00004e89d00002e9,0x00004f09e00002e6,0x00004f89f00002e3,
   0x0000500a00000220,0x0000508a10000225,0x0000510a2000022a,0x0000518a3000022f,0x0000520a40000234,0x0000528a50000231,0x0000530a6000023e,0x0000538a7000023b,0x0000540a80000208,0x0000548a9000020d,0x0000550aa0000202,0x0000558ab0000207,0x0000560ac000021c,0x0000568ad0000219,0x0000570ae0000216,0x0000578af0000213,
   0x0000580b00000270,0x0000588b10000275,0x0000590b2000027a,0x0000598b3000027f,0x00005a0b40000264,0x00005a8b50000261,0x00005b0b6000026e,0x00005b8b7000026b,0x00005c0b80000258,0x00005c8b9000025d,0x00005d0ba0000252,0x00005d8bb0000257,0x00005e0bc000024c,0x00005e8bd0000249,0x00005f0be0000246,0x00005f8bf0000243,
   0x0000600c000003c0,0x0000608c100003c5,0x0000610c200003ca,0x0000618c300003cf,0x0000620c400003d4,0x0000628c500003d1,0x0000630c600003de,0x0000638c700003db,0x0000640c800003e8,0x0000648c900003ed,0x0000650ca00003e2,0x0000658cb00003e7,0x0000660cc00003fc,0x0000668cd00003f9,0x0000670ce00003f6,0x0000678cf00003f3,
   0x0000680d00000390,0x0000688d10000395,0x0000690d2000039a,0x0000698d3000039f,0x00006a0d40000384,0x00006a8d50000381,0x00006b0d6000038e,0x00006b8d7000038b,0x00006c0d800003b8,0x00006c8d900003bd,0x00006d0da00003b2,0x00006d8db00003b7,0x00006e0dc00003ac,0x00006e8dd00003a9,0x00006f0de00003a6,0x00006f8df00003a3,
   0x0000700e00000360,0x0000708e10000365,0x0000710e2000036a,0x0000718e3000036f,0x0000720e40000374,0x0000728e50000371,0x0000730e6000037e,0x0000738e7000037b,0x0000740e80000348,0x0000748e9000034d,0x0000750ea0000342,0x0000758eb0000347,0x0000760ec000035c,0x0000768ed0000359,0x0000770ee0000356,0x0000778ef0000353,
   0x0000780f00000330,0x0000788f10000335,0x0000790f2000033a,0x0000798f3000033f,0x00007a0f40000324,0x00007a8f50000321,0x00007b0f6000032e,0x00007b8f7000032b,0x00007c0f80000318,0x00007c8f9000031d,0x00007d0fa0000312,0x00007d8fb0000317,0x00007e0fc000030c,0x00007e8fd0000309,0x00007f0fe0000306,0x00007f8ff0000303,
};//对应于5阶矩阵的mixcolun中的x^128 + x^39+ x^28 + x^2 + 1乘以\alpha^8



static const uint64_t reduction_table_1_MD5[2] GARNET_FAST_TABLE =
{
    0x0, 0x0000200010000005,
};//对应于5阶矩阵的mixdiagonal中的x^128+x^45+x^28+x^2+1乘以\alpha

static const uint64_t reduction_table_2_MD5[4] GARNET_FAST_TABLE =
{
    0x0000000000000000,0x0000200010000005,0x000040002000000a,0x000060003000000f,
};//对应于5阶矩阵的mixdiagonal中的x^128+x^45+x^28+x^2+1乘以\alpha^2

static const uint64_t reduction_table_4_MD5[16] GARNET_FAST_TABLE =
{
    0x0000000000000000,0x0000200010000005,0x000040002000000a,0x000060003000000f,0x0000800040000014,0x0000a00050000011,0x0000c0006000001e,0x0000e0007000001b,
    0x0001000080000028,0x000120009000002d,0x00014000a0000022,0x00016000b0000027,0x00018000c000003c,0x0001a000d0000039,0x0001c000e0000036,0x0001e000f0000033,
};//对应于5阶矩阵的mixdiagonal中的x^128+x^45+x^28+x^2+1乘以\alpha^4


static const uint64_t reduction_table_8_MD5[256] GARNET_FAST_TABLE =
{
0x0000000000000000,0x0000200010000005,0x000040002000000a,0x000060003000000f,0x0000800040000014,0x0000a00050000011,0x0000c0006000001e,0x0000e0007000001b,0x0001000080000028,0x000120009000002d,0x00014000a0000022,0x00016000b0000027,0x00018000c000003c,0x0001a000d0000039,0x0001c000e0000036,0x0001e000f0000033,
0x0002000100000050,0x0002200110000055,0x000240012000005a,0x000260013000005f,0x0002800140000044,0x0002a00150000041,0x0002c0016000004e,0x0002e0017000004b,0x0003000180000078,0x000320019000007d,0x00034001a0000072,0x00036001b0000077,0x00038001c000006c,0x0003a001d0000069,0x0003c001e0000066,0x0003e001f0000063,
0x00040002000000a0,0x00042002100000a5,0x00044002200000aa,0x00046002300000af,0x00048002400000b4,0x0004a002500000b1,0x0004c002600000be,0x0004e002700000bb,0x0005000280000088,0x000520029000008d,0x00054002a0000082,0x00056002b0000087,0x00058002c000009c,0x0005a002d0000099,0x0005c002e0000096,0x0005e002f0000093,
0x00060003000000f0,0x00062003100000f5,0x00064003200000fa,0x00066003300000ff,0x00068003400000e4,0x0006a003500000e1,0x0006c003600000ee,0x0006e003700000eb,0x00070003800000d8,0x00072003900000dd,0x00074003a00000d2,0x00076003b00000d7,0x00078003c00000cc,0x0007a003d00000c9,0x0007c003e00000c6,0x0007e003f00000c3,
0x0008000400000140,0x0008200410000145,0x000840042000014a,0x000860043000014f,0x0008800440000154,0x0008a00450000151,0x0008c0046000015e,0x0008e0047000015b,0x0009000480000168,0x000920049000016d,0x00094004a0000162,0x00096004b0000167,0x00098004c000017c,0x0009a004d0000179,0x0009c004e0000176,0x0009e004f0000173,
0x000a000500000110,0x000a200510000115,0x000a40052000011a,0x000a60053000011f,0x000a800540000104,0x000aa00550000101,0x000ac0056000010e,0x000ae0057000010b,0x000b000580000138,0x000b20059000013d,0x000b4005a0000132,0x000b6005b0000137,0x000b8005c000012c,0x000ba005d0000129,0x000bc005e0000126,0x000be005f0000123,
0x000c0006000001e0,0x000c2006100001e5,0x000c4006200001ea,0x000c6006300001ef,0x000c8006400001f4,0x000ca006500001f1,0x000cc006600001fe,0x000ce006700001fb,0x000d0006800001c8,0x000d2006900001cd,0x000d4006a00001c2,0x000d6006b00001c7,0x000d8006c00001dc,0x000da006d00001d9,0x000dc006e00001d6,0x000de006f00001d3,
0x000e0007000001b0,0x000e2007100001b5,0x000e4007200001ba,0x000e6007300001bf,0x000e8007400001a4,0x000ea007500001a1,0x000ec007600001ae,0x000ee007700001ab,0x000f000780000198,0x000f20079000019d,0x000f4007a0000192,0x000f6007b0000197,0x000f8007c000018c,0x000fa007d0000189,0x000fc007e0000186,0x000fe007f0000183,
0x0010000800000280,0x0010200810000285,0x001040082000028a,0x001060083000028f,0x0010800840000294,0x0010a00850000291,0x0010c0086000029e,0x0010e0087000029b,0x00110008800002a8,0x00112008900002ad,0x00114008a00002a2,0x00116008b00002a7,0x00118008c00002bc,0x0011a008d00002b9,0x0011c008e00002b6,0x0011e008f00002b3,
0x00120009000002d0,0x00122009100002d5,0x00124009200002da,0x00126009300002df,0x00128009400002c4,0x0012a009500002c1,0x0012c009600002ce,0x0012e009700002cb,0x00130009800002f8,0x00132009900002fd,0x00134009a00002f2,0x00136009b00002f7,0x00138009c00002ec,0x0013a009d00002e9,0x0013c009e00002e6,0x0013e009f00002e3,
0x0014000a00000220,0x0014200a10000225,0x0014400a2000022a,0x0014600a3000022f,0x0014800a40000234,0x0014a00a50000231,0x0014c00a6000023e,0x0014e00a7000023b,0x0015000a80000208,0x0015200a9000020d,0x0015400aa0000202,0x0015600ab0000207,0x0015800ac000021c,0x0015a00ad0000219,0x0015c00ae0000216,0x0015e00af0000213,
0x0016000b00000270,0x0016200b10000275,0x0016400b2000027a,0x0016600b3000027f,0x0016800b40000264,0x0016a00b50000261,0x0016c00b6000026e,0x0016e00b7000026b,0x0017000b80000258,0x0017200b9000025d,0x0017400ba0000252,0x0017600bb0000257,0x0017800bc000024c,0x0017a00bd0000249,0x0017c00be0000246,0x0017e00bf0000243,
0x0018000c000003c0,0x0018200c100003c5,0x0018400c200003ca,0x0018600c300003cf,0x0018800c400003d4,0x0018a00c500003d1,0x0018c00c600003de,0x0018e00c700003db,0x0019000c800003e8,0x0019200c900003ed,0x0019400ca00003e2,0x0019600cb00003e7,0x0019800cc00003fc,0x0019a00cd00003f9,0x0019c00ce00003f6,0x0019e00cf00003f3,
0x001a000d00000390,0x001a200d10000395,0x001a400d2000039a,0x001a600d3000039f,0x001a800d40000384,0x001aa00d50000381,0x001ac00d6000038e,0x001ae00d7000038b,0x001b000d800003b8,0x001b200d900003bd,0x001b400da00003b2,0x001b600db00003b7,0x001b800dc00003ac,0x001ba00dd00003a9,0x001bc00de00003a6,0x001be00df00003a3,
0x001c000e00000360,0x001c200e10000365,0x001c400e2000036a,0x001c600e3000036f,0x001c800e40000374,0x001ca00e50000371,0x001cc00e6000037e,0x001ce00e7000037b,0x001d000e80000348,0x001d200e9000034d,0x001d400ea0000342,0x001d600eb0000347,0x001d800ec000035c,0x001da00ed0000359,0x001dc00ee0000356,0x001de00ef0000353,
0x001e000f00000330,0x001e200f10000335,0x001e400f2000033a,0x001e600f3000033f,0x001e800f40000324,0x001ea00f50000321,0x001ec00f6000032e,0x001ee00f7000032b,0x001f000f80000318,0x001f200f9000031d,0x001f400fa0000312,0x001f600fb0000317,0x001f800fc000030c,0x001fa00fd0000309,0x001fc00fe0000306,0x001fe00ff0000303,
}; //对应于5阶矩阵的mixdiagonal中的x^128+x^45+x^28+x^2+1乘以\alpha^8


/**
 * 4x4 专用优化宏 (GF128 Mul Alpha)
 * 逻辑：
 * 1. 提取 128 位的最高位 (m[3] 的 MSB) 作为索引。
 * 2. 执行级联左移 1 位。
 * 3. 将 64 位还原项 (u64) 分别异或到低 64 位的两个 32 位字 (m[1], m[0]) 中。
 */
#define GF128_MUL_ALPHA_MC4_FIXED(a, table) ({ \
    uint128_t res; \
    u32 m3 = (a).u.m[3], m2 = (a).u.m[2], m1 = (a).u.m[1], m0 = (a).u.m[0]; \
    /* 获取最高位 */ \
    u32 top_bit = m3 >> 31; \
    /* 从表中获取 64 位还原项 */ \
    u64 poly = (table)[top_bit & 0x1]; \
    u32 poly_hi = (u32)(poly >> 32); \
    u32 poly_lo = (u32)(poly); \
    \
    /* 128 位级联左移 */ \
    res.u.m[3] = (m3 << 1) | (m2 >> 31); \
    res.u.m[2] = (m2 << 1) | (m1 >> 31); \
    /* 重点：还原项异或到 v[0] 的两个字 m[1] 和 m[0] */ \
    res.u.m[1] = ((m1 << 1) | (m0 >> 31)) ^ poly_hi; \
    res.u.m[0] = (m0 << 1) ^ poly_lo; \
    res; \
})

#define MIX4_STEP(s0, s1, s2, s3, table) do { \
    uint128_t a0, a1, a2, a3, S; \
    /* 1. 计算 Alpha 乘法 (A_i = alpha * s_i) */ \
    a0 = GF128_MUL_ALPHA_MC4_FIXED(s0, table); \
    a1 = GF128_MUL_ALPHA_MC4_FIXED(s1, table); \
    a2 = GF128_MUL_ALPHA_MC4_FIXED(s2, table); \
    a3 = GF128_MUL_ALPHA_MC4_FIXED(s3, table); \
    \
    /* 2. 计算四个元素的总异或和 S */ \
    S.u.m[0] = s0.u.m[0] ^ s1.u.m[0] ^ s2.u.m[0] ^ s3.u.m[0]; \
    S.u.m[1] = s0.u.m[1] ^ s1.u.m[1] ^ s2.u.m[1] ^ s3.u.m[1]; \
    S.u.m[2] = s0.u.m[2] ^ s1.u.m[2] ^ s2.u.m[2] ^ s3.u.m[2]; \
    S.u.m[3] = s0.u.m[3] ^ s1.u.m[3] ^ s2.u.m[3] ^ s3.u.m[3]; \
    \
    /* 3. 根据公式计算结果并写回 */ \
    /* t0 = a0 ^ a1 ^ S ^ s0 */ \
    uint128_t t; \
    t.u.m[0] = a0.u.m[0] ^ a1.u.m[0] ^ S.u.m[0] ^ s0.u.m[0]; \
    t.u.m[1] = a0.u.m[1] ^ a1.u.m[1] ^ S.u.m[1] ^ s0.u.m[1]; \
    t.u.m[2] = a0.u.m[2] ^ a1.u.m[2] ^ S.u.m[2] ^ s0.u.m[2]; \
    t.u.m[3] = a0.u.m[3] ^ a1.u.m[3] ^ S.u.m[3] ^ s0.u.m[3]; \
    s0 = t; \
    /* t1 = a1 ^ a2 ^ S ^ s1 */ \
    t.u.m[0] = a1.u.m[0] ^ a2.u.m[0] ^ S.u.m[0] ^ s1.u.m[0]; \
    t.u.m[1] = a1.u.m[1] ^ a2.u.m[1] ^ S.u.m[1] ^ s1.u.m[1]; \
    t.u.m[2] = a1.u.m[2] ^ a2.u.m[2] ^ S.u.m[2] ^ s1.u.m[2]; \
    t.u.m[3] = a1.u.m[3] ^ a2.u.m[3] ^ S.u.m[3] ^ s1.u.m[3]; \
    s1 = t; \
    /* t2 = a2 ^ a3 ^ S ^ s2 */ \
    t.u.m[0] = a2.u.m[0] ^ a3.u.m[0] ^ S.u.m[0] ^ s2.u.m[0]; \
    t.u.m[1] = a2.u.m[1] ^ a3.u.m[1] ^ S.u.m[1] ^ s2.u.m[1]; \
    t.u.m[2] = a2.u.m[2] ^ a3.u.m[2] ^ S.u.m[2] ^ s2.u.m[2]; \
    t.u.m[3] = a2.u.m[3] ^ a3.u.m[3] ^ S.u.m[3] ^ s2.u.m[3]; \
    s2 = t; \
    /* t3 = a3 ^ a0 ^ S ^ s3 */ \
    t.u.m[0] = a3.u.m[0] ^ a0.u.m[0] ^ S.u.m[0] ^ s3.u.m[0]; \
    t.u.m[1] = a3.u.m[1] ^ a0.u.m[1] ^ S.u.m[1] ^ s3.u.m[1]; \
    t.u.m[2] = a3.u.m[2] ^ a0.u.m[2] ^ S.u.m[2] ^ s3.u.m[2]; \
    t.u.m[3] = a3.u.m[3] ^ a0.u.m[3] ^ S.u.m[3] ^ s3.u.m[3]; \
    s3 = t; \
} while(0)
GARNET_FAST_FUNC
void mix_column_4x4_st(struct uint128_t state[STATE_4x4]) {
    for (int col = 0; col < 4; col++) {
        MIX4_STEP(state[col], state[col+4], state[col+8], state[col+12], reduction_table_1_MC4);
    }
}

GARNET_FAST_FUNC
void mix_diagonal_4x4_st(struct uint128_t state[STATE_4x4]) {
    // 组1: (0, 5, 10, 15)
    MIX4_STEP(state[0], state[5], state[10], state[15], reduction_table_1_MD4);
    
    // 组2: (1, 6, 11, 12)
    MIX4_STEP(state[1], state[6], state[11], state[12], reduction_table_1_MD4);
    
    // 组3: (2, 7, 8, 13)
    MIX4_STEP(state[2], state[7], state[8], state[13], reduction_table_1_MD4);
    
    // 组4: (3, 4, 9, 14)
    MIX4_STEP(state[3], state[4], state[9], state[14], reduction_table_1_MD4);
}

static inline uint128_t gf128_reduce_5x5_internal(uint128_t a, const u64* table, int n) {
    uint128_t res;
    u32 m3 = a.u.m[3], m2 = a.u.m[2], m1 = a.u.m[1], m0 = a.u.m[0];
    
    u32 idx = m3 >> (32 - n);
    u64 poly = table[idx];
    u32 poly_hi = (u32)(poly >> 32);
    u32 poly_lo = (u32)(poly);

    res.u.m[3] = (m3 << n) | (m2 >> (32 - n));
    res.u.m[2] = (m2 << n) | (m1 >> (32 - n));
    res.u.m[1] = ((m1 << n) | (m0 >> (32 - n))) ^ poly_hi;
    res.u.m[0] = (m0 << n) ^ poly_lo;
    
    return res;
}
#define MIX5_CORE(s0, s1, s2, s3, s4, T1, T2, T4, T8) do { \
    uint128_t a1[5], a2[5], a4[5], a8[5]; \
    /* 预计算每个元素的幂次版本 */ \
    a1[0] = gf128_reduce_5x5_internal(s0, T1, 1); \
    a2[0] = gf128_reduce_5x5_internal(s0, T2, 2); \
    a4[0] = gf128_reduce_5x5_internal(s0, T4, 4); \
    a8[0] = gf128_reduce_5x5_internal(s0, T8, 8); \
    \
    a1[1] = gf128_reduce_5x5_internal(s1, T1, 1); \
    a2[1] = gf128_reduce_5x5_internal(s1, T2, 2); \
    a4[1] = gf128_reduce_5x5_internal(s1, T4, 4); \
    a8[1] = gf128_reduce_5x5_internal(s1, T8, 8); \
    \
    a1[2] = gf128_reduce_5x5_internal(s2, T1, 1); \
    a2[2] = gf128_reduce_5x5_internal(s2, T2, 2); \
    a4[2] = gf128_reduce_5x5_internal(s2, T4, 4); \
    a8[2] = gf128_reduce_5x5_internal(s2, T8, 8); \
    \
    a1[3] = gf128_reduce_5x5_internal(s3, T1, 1); \
    a2[3] = gf128_reduce_5x5_internal(s3, T2, 2); \
    a4[3] = gf128_reduce_5x5_internal(s3, T4, 4); \
    a8[3] = gf128_reduce_5x5_internal(s3, T8, 8); \
    \
    a1[4] = gf128_reduce_5x5_internal(s4, T1, 1); \
    a2[4] = gf128_reduce_5x5_internal(s4, T2, 2); \
    a4[4] = gf128_reduce_5x5_internal(s4, T4, 4); \
    a8[4] = gf128_reduce_5x5_internal(s4, T8, 8); \
    \
    uint128_t t; \
    /* t0 = s0 ^ a1[1] ^ a2[2] ^ a4[3] ^ a8[4] */ \
    t.u.m[0] = s0.u.m[0] ^ a1[1].u.m[0] ^ a2[2].u.m[0] ^ a4[3].u.m[0] ^ a8[4].u.m[0]; \
    t.u.m[1] = s0.u.m[1] ^ a1[1].u.m[1] ^ a2[2].u.m[1] ^ a4[3].u.m[1] ^ a8[4].u.m[1]; \
    t.u.m[2] = s0.u.m[2] ^ a1[1].u.m[2] ^ a2[2].u.m[2] ^ a4[3].u.m[2] ^ a8[4].u.m[2]; \
    t.u.m[3] = s0.u.m[3] ^ a1[1].u.m[3] ^ a2[2].u.m[3] ^ a4[3].u.m[3] ^ a8[4].u.m[3]; \
    uint128_t out0 = t; \
    \
    /* t1 = a8[0] ^ s1 ^ a1[2] ^ a2[3] ^ a4[4] */ \
    t.u.m[0] = a8[0].u.m[0] ^ s1.u.m[0] ^ a1[2].u.m[0] ^ a2[3].u.m[0] ^ a4[4].u.m[0]; \
    t.u.m[1] = a8[0].u.m[1] ^ s1.u.m[1] ^ a1[2].u.m[1] ^ a2[3].u.m[1] ^ a4[4].u.m[1]; \
    t.u.m[2] = a8[0].u.m[2] ^ s1.u.m[2] ^ a1[2].u.m[2] ^ a2[3].u.m[2] ^ a4[4].u.m[2]; \
    t.u.m[3] = a8[0].u.m[3] ^ s1.u.m[3] ^ a1[2].u.m[3] ^ a2[3].u.m[3] ^ a4[4].u.m[3]; \
    uint128_t out1 = t; \
    \
    /* t2 = a4[0] ^ a8[1] ^ s2 ^ a1[3] ^ a2[4] */ \
    t.u.m[0] = a4[0].u.m[0] ^ a8[1].u.m[0] ^ s2.u.m[0] ^ a1[3].u.m[0] ^ a2[4].u.m[0]; \
    t.u.m[1] = a4[0].u.m[1] ^ a8[1].u.m[1] ^ s2.u.m[1] ^ a1[3].u.m[1] ^ a2[4].u.m[1]; \
    t.u.m[2] = a4[0].u.m[2] ^ a8[1].u.m[2] ^ s2.u.m[2] ^ a1[3].u.m[2] ^ a2[4].u.m[2]; \
    t.u.m[3] = a4[0].u.m[3] ^ a8[1].u.m[3] ^ s2.u.m[3] ^ a1[3].u.m[3] ^ a2[4].u.m[3]; \
    uint128_t out2 = t; \
    \
    /* t3 = a2[0] ^ a4[1] ^ a8[2] ^ s3 ^ a1[4] */ \
    t.u.m[0] = a2[0].u.m[0] ^ a4[1].u.m[0] ^ a8[2].u.m[0] ^ s3.u.m[0] ^ a1[4].u.m[0]; \
    t.u.m[1] = a2[0].u.m[1] ^ a4[1].u.m[1] ^ a8[2].u.m[1] ^ s3.u.m[1] ^ a1[4].u.m[1]; \
    t.u.m[2] = a2[0].u.m[2] ^ a4[1].u.m[2] ^ a8[2].u.m[2] ^ s3.u.m[2] ^ a1[4].u.m[2]; \
    t.u.m[3] = a2[0].u.m[3] ^ a4[1].u.m[3] ^ a8[2].u.m[3] ^ s3.u.m[3] ^ a1[4].u.m[3]; \
    uint128_t out3 = t; \
    \
    /* t4 = a1[0] ^ a2[1] ^ a4[2] ^ a8[3] ^ s4 */ \
    t.u.m[0] = a1[0].u.m[0] ^ a2[1].u.m[0] ^ a4[2].u.m[0] ^ a8[3].u.m[0] ^ s4.u.m[0]; \
    t.u.m[1] = a1[0].u.m[1] ^ a2[1].u.m[1] ^ a4[2].u.m[1] ^ a8[3].u.m[1] ^ s4.u.m[1]; \
    t.u.m[2] = a1[0].u.m[2] ^ a2[1].u.m[2] ^ a4[2].u.m[2] ^ a8[3].u.m[2] ^ s4.u.m[2]; \
    t.u.m[3] = a1[0].u.m[3] ^ a2[1].u.m[3] ^ a4[2].u.m[3] ^ a8[3].u.m[3] ^ s4.u.m[3]; \
    uint128_t out4 = t; \
    \
    s0 = out0; s1 = out1; s2 = out2; s3 = out3; s4 = out4; \
} while(0)
void mix_column_5x5_st(struct uint128_t state[STATE_5x5]) {
    for (int col = 0; col < 5; col++) {
        MIX5_CORE(state[col], state[col+5], state[col+10], state[col+15], state[col+20], 
                  reduction_table_1_MC5, reduction_table_2_MC5, 
                  reduction_table_4_MC5, reduction_table_8_MC5);
    }
}

GARNET_FAST_FUNC
void mix_diagonal_5x5_st(struct uint128_t state[STATE_5x5]) {
    // 组1
    MIX5_CORE(state[0], state[6], state[12], state[18], state[24], 
              reduction_table_1_MD5, reduction_table_2_MD5, 
              reduction_table_4_MD5, reduction_table_8_MD5);
    // 组2
    MIX5_CORE(state[1], state[7], state[13], state[19], state[20], 
              reduction_table_1_MD5, reduction_table_2_MD5, 
              reduction_table_4_MD5, reduction_table_8_MD5);
    // 组3
    MIX5_CORE(state[2], state[8], state[14], state[15], state[21], 
              reduction_table_1_MD5, reduction_table_2_MD5, 
              reduction_table_4_MD5, reduction_table_8_MD5);
    // 组4
    MIX5_CORE(state[3], state[9], state[10], state[16], state[22], 
              reduction_table_1_MD5, reduction_table_2_MD5, 
              reduction_table_4_MD5, reduction_table_8_MD5);
    // 组5
    MIX5_CORE(state[4], state[5], state[11], state[17], state[23], 
              reduction_table_1_MD5, reduction_table_2_MD5, 
              reduction_table_4_MD5, reduction_table_8_MD5);
}


GARNET_FAST_FUNC
void subword_4x4_st(struct uint128_t state[STATE_4x4], const struct uint128_t *key)
{
    // 将 key 转换为字节指针，满足 aesround 签名
    unsigned char *rk = (unsigned char *)key->u.n;

    for (int i = 0; i < STATE_4x4; i++)
    {
        struct uint128_t out_tmp;       
        aesround(out_tmp.u.n, state[i].u.n, rk);
        state[i] = out_tmp;
    }
}

// 5x5 版本同理
GARNET_FAST_FUNC
void subword_5x5_st(struct uint128_t state[STATE_5x5], const struct uint128_t *key)
{
    unsigned char *rk = (unsigned char *)key->u.n;
    for (int i = 0; i < STATE_5x5; i++)
    {
        struct uint128_t out_tmp;
        aesround(out_tmp.u.n, state[i].u.n, rk);
        state[i] = out_tmp;
    }
}

// 定义底层 64 位常量异或宏
// 假设 m[0] 是最低 32 位，m[1] 是次低 32 位 (对应 v[0])
#define APPLY_U64_CONST(s_ptr, c_u64) do { \
    (s_ptr)->u.m[0] ^= (u32)(c_u64);       \
    (s_ptr)->u.m[1] ^= (u32)((c_u64) >> 32); \
} while(0)

GARNET_FAST_FUNC
void add_constant_4x4_st(struct uint128_t state[STATE_4x4], int round)
{
    // 仅针对特定索引进行 64 位异或，不触碰高 64 位 (m[2], m[3])
    APPLY_U64_CONST(&state[7],  Fibonacci_4x4[round][0]);
    APPLY_U64_CONST(&state[11], Fibonacci_4x4[round][1]);
    APPLY_U64_CONST(&state[15], Fibonacci_4x4[round][2]);
}

GARNET_FAST_FUNC
void add_constant_5x5_st(struct uint128_t state[STATE_5x5], int round)
{
    APPLY_U64_CONST(&state[9],  Fibonacci_5x5[round][0]);
    APPLY_U64_CONST(&state[14], Fibonacci_5x5[round][1]);
    APPLY_U64_CONST(&state[19], Fibonacci_5x5[round][2]);
    APPLY_U64_CONST(&state[24], Fibonacci_5x5[round][3]);
}

// 完整的置换P（4x4状态）
GARNET_FAST_FUNC
void permutation_p_4x4_st(struct  uint128_t state[STATE_4x4], struct  uint128_t *key, int round)
{
    subword_4x4_st(state, key);
    mix_column_4x4_st(state);
    add_constant_4x4_st(state, round);
    mix_diagonal_4x4_st(state);
}

// 完整的置换P（5x5状态）
GARNET_FAST_FUNC
void permutation_p_5x5_st(struct  uint128_t state[STATE_5x5], struct  uint128_t *key, int round)
{
    subword_5x5_st(state, key);
    mix_column_5x5_st(state);
    add_constant_5x5_st(state, round);
    mix_diagonal_5x5_st(state);
}

GARNET_FAST_FUNC
void pad_message(unsigned char* msg, u64 bit_len)
{
    int  byte_num = 0, bit_idx = 0;
    u8 mask = 0, mask1 = 0;

    if ((bit_len % 8) == 0)byte_num = bit_len / 8;
    else byte_num = bit_len / 8 + 1;

    bit_idx = bit_len % 8;

    if ((bit_len % 8) == 0)
    {
        msg[byte_num] = 0x00;
    }
    else
    {
        mask1 = ~((1 << (7 - bit_idx + 1)) - 1);
        msg[byte_num - 1] &= mask1;
    }


    if ((bit_len % 8) != 0)
    {
        msg[byte_num - 1] |= 1 << (7 - bit_idx); //这里是填充的1
        mask = ~((1 << (7 - bit_idx)) - 1);
        //printf("mask is %02x\n", mask);
        msg[byte_num - 1] &= mask;
    }
    else
    {
        msg[byte_num] = 0x80;
    }


}



// ---------------------------------------------------------
//  统一架构的吸收与提取适配器
// ---------------------------------------------------------

static inline void absorb_dynamic_msg(struct uint128_t* state, struct uint128_t* msg, int is_5x5, int rate_blocks,int is_sponge_dm, uint64_t counter) 
{
    if (counter == COUNTER_1024_W2048_DM) {
        const uint8_t m[16] = {0, 2, 4, 6, 7, 8, 10, 11, 12, 13, 14, 17, 18, 20, 22, 24};
        for (int k = 0; k < 16; k++) state[m[k]] = xor128(state[m[k]], msg[k]);
        return;
    }
    if (is_5x5) {
        // --- 5x5 Matrix Patterns (Garnet-1024) ---
       // =======================================================
        // 5x5 矩阵阵型组 (专属 Garnet-1024 家族)
        // =======================================================
        if (rate_blocks == 8) {
            // 阵型 8：Garnet-1024 / w=1024 bit (残缺 X 型)
            state[0]  = xor128(state[0],  msg[0]);
            state[4]  = xor128(state[4],  msg[1]);
            state[6]  = xor128(state[6],  msg[2]);
            state[8]  = xor128(state[8],  msg[3]);
            state[12] = xor128(state[12], msg[4]);
            state[18] = xor128(state[18], msg[5]);
            state[20] = xor128(state[20], msg[6]);
            state[24] = xor128(state[24], msg[7]);
        } 
        else if (rate_blocks == 9) {
            // 阵型 9：Garnet-1024 / w=1152 bit (完美全 X 型)
            state[0]  = xor128(state[0],  msg[0]);
            state[4]  = xor128(state[4],  msg[1]);
            state[6]  = xor128(state[6],  msg[2]);
            state[8]  = xor128(state[8],  msg[3]);
            state[12] = xor128(state[12], msg[4]);
            state[16] = xor128(state[16], msg[5]); 
            state[18] = xor128(state[18], msg[6]);
            state[20] = xor128(state[20], msg[7]);
            state[24] = xor128(state[24], msg[8]);
        }
    } 
    else {
        // --- 4x4 Matrix Patterns ---
        if (rate_blocks == 4) {
            // Pattern: w=512 bit (Main Diagonal)
            state[0]  = xor128(state[0],  msg[0]);
            state[5]  = xor128(state[5],  msg[1]);
            state[10] = xor128(state[10], msg[2]);
            state[15] = xor128(state[15], msg[3]);
        } 
        else if (rate_blocks == 5) {
            // Pattern: w=640 bit
            state[0]  = xor128(state[0],  msg[0]);
            state[5]  = xor128(state[5],  msg[1]);
            state[10] = xor128(state[10], msg[2]);
            state[12] = xor128(state[12], msg[3]);
            state[15] = xor128(state[15], msg[4]);
        } 
        else if (rate_blocks == 6) {
            // Pattern: w=768 bit
            state[0]  = xor128(state[0],  msg[0]);
            state[3]  = xor128(state[3],  msg[1]);
            state[5] = xor128(state[5], msg[2]);
            state[10] = xor128(state[10], msg[3]);
            state[12] = xor128(state[12], msg[4]);
            state[15]  = xor128(state[15],  msg[5]);
        } 
        else if (rate_blocks == 7) {
          // Pattern: w=896 bit
            if (!is_sponge_dm) {
                // 情况 1: Garnet-512 / w=896 bit
                // 阵型包含 S12
            
    state[0] = xor128(state[0], msg[0]);
    state[3] = xor128(state[3], msg[1]);
    state[5] = xor128(state[5], msg[2]);
    state[6] = xor128(state[6], msg[3]);
    state[10] = xor128(state[10], msg[4]);
    state[12] = xor128(state[12], msg[5]);
    state[15] = xor128(state[15], msg[6]);
            } else {
                // 情况 2: Garnet-1024a (Sponge-DM) / w=896 bit
                // 阵型包含 S9，从而给 Capacity 腾出 S12

                
                state[0]  = xor128(state[0],  msg[0]);
                state[3]  = xor128(state[3],  msg[1]);
                state[5]  = xor128(state[5],  msg[2]);
                state[6]  = xor128(state[6],  msg[3]);
                state[9]  = xor128(state[9],  msg[4]); // 1024a 专属
                state[10] = xor128(state[10], msg[5]);
                state[15] = xor128(state[15], msg[6]);
                
            }
        } 
        else if (rate_blocks == 8) {
            // Pattern: w=1024 bit (Full 4x4 "X")
      state[0] = xor128(state[0], msg[0]);
    state[3] = xor128(state[3], msg[1]);
    state[5] = xor128(state[5], msg[2]);
    state[6] = xor128(state[6], msg[3]);
    state[9] = xor128(state[9], msg[4]);
    state[10] = xor128(state[10], msg[5]);
    state[12] = xor128(state[12], msg[6]);
    state[15] = xor128(state[15], msg[7]);
        }
    }
}



static inline void absorb_dynamic_length(struct uint128_t* state, uint64_t mlength, int is_5x5) 
{
    struct uint128_t msglen;
    msglen.u.v[1] = 0; 
    msglen.u.v[0] = mlength;

    if (is_5x5) {
        state[0]  = xor128(state[0],  msglen);
        state[6]  = xor128(state[6],  msglen);
        state[12] = xor128(state[12], msglen);
        state[18] = xor128(state[18], msglen);
        state[24] = xor128(state[24], msglen);
    } else {
        state[0]  = xor128(state[0],  msglen);
        state[5]  = xor128(state[5],  msglen);
        state[10] = xor128(state[10], msglen);
        state[15] = xor128(state[15], msglen);
    }
}

static inline void extract_digest_dynamic(u8* digest, struct uint128_t* state, int is_5x5, uint32_t target_bits) 
{
    if (is_5x5) {
        // ==========================================
        // 模式: Garnet-1024 (5x5 矩阵)
        // 提取 8 个字 (1024 位)
        // 阵型: S0, S4, S8, S10, S12, S19, S21, S23
        // ==========================================
        memcpy(digest,       &state[0],  16);
        memcpy(digest + 16,  &state[4],  16);
        memcpy(digest + 32,  &state[8],  16);
        memcpy(digest + 48,  &state[10], 16);
        memcpy(digest + 64,  &state[12], 16);
        memcpy(digest + 80,  &state[19], 16);
        memcpy(digest + 96,  &state[21], 16);
        memcpy(digest + 112, &state[23], 16);
    } 
    else {
        // ==========================================
        // 4x4 矩阵下的三种变体提取逻辑
        // ==========================================
        if (target_bits == 512) {
            // 模式: Garnet-512
            // 提取 4 个字 (512 位)
            // 阵型: S0, S7, S9, S14
            memcpy(digest,      &state[0],  16);
            memcpy(digest + 16, &state[7],  16);
            memcpy(digest + 32, &state[9],  16);
            memcpy(digest + 48, &state[14], 16);
        } 
        else if (target_bits == 768) {
            // 模式: Garnet-768
            // 提取 6 个字 (768 位)
            // 阵型: S0, S3, S6, S9, S13, S15
            memcpy(digest,      &state[0],  16);
            memcpy(digest + 16, &state[3],  16);
            memcpy(digest + 32, &state[6],  16);
            memcpy(digest + 48, &state[9],  16);
            memcpy(digest + 64, &state[13], 16);
            memcpy(digest + 80, &state[15], 16);
        } 
        else if (target_bits == 1024) {
            // 模式: Garnet-1024a
            // 提取 8 个字 (1024 位)
            // 阵型: S0, S2, S5, S7, S9, S11, S12, S14
            memcpy(digest,       &state[0],  16);
            memcpy(digest + 16,  &state[2],  16);
            memcpy(digest + 32,  &state[5],  16);
            memcpy(digest + 48,  &state[7],  16);
            memcpy(digest + 64,  &state[9],  16);
            memcpy(digest + 80,  &state[11], 16);
            memcpy(digest + 96,  &state[12], 16);
            memcpy(digest + 112, &state[14], 16);
        }
    }
}

/**
 * 5x5 DM按压缩顺序（0-8）读取，4x4 DM 按原始索引（1,2,4...）读取
 */
static inline void dynamic_feed_forward(struct uint128_t* state, struct uint128_t* snapshot, uint64_t counter) 
{
    if (IS_FLAGSHIP_5x5(counter)) {
        const uint8_t c[9] = {1, 3, 5, 9, 15, 16, 19, 21, 23};
        for (int k = 0; k < 9; k++) state[c[k]] = xor128(state[c[k]], snapshot[k]);
    } 
    else {
        // 4x4 DM 逻辑（Counter 8）: 备份的是 state[k] = snapshot[k]
        const uint8_t c[9] = {1, 2, 4, 7, 8, 11, 12, 13, 14};
        for (int k = 0; k < 9; k++) state[c[k]] = xor128(state[c[k]], snapshot[c[k]]);
    }
}
/**
 * @brief 动态容量恢复器 (Sponge-DM 核心)
 * @param state 当前的置换后状态
 * @param snapshot 置换前的状态快照 (即 cap 数组)
 */
static inline void absorb_dynamic_capacity(struct uint128_t* state, struct uint128_t* snapshot, int is_5x5) 
{
    if (is_5x5) {
          const uint8_t c[9] = {1, 3, 5, 9, 15, 16, 19, 21, 23};
        for (int k = 0; k < 9; k++) state[c[k]] = xor128(state[c[k]], snapshot[k]);
    } 
    else {
        // --- 4x4 矩阵的容量恢复逻辑 (精确匹配原作者 keep_capacity5_4x4_st) ---
        // 只要是当时 keep_capacity 存下来的格子，现在全部 XOR 回去
        state[1]  = xor128(state[1],  snapshot[1]);
        state[2]  = xor128(state[2],  snapshot[2]);
        state[4]  = xor128(state[4],  snapshot[4]);
        state[7]  = xor128(state[7],  snapshot[7]);
        state[8]  = xor128(state[8],  snapshot[8]);
        state[11] = xor128(state[11], snapshot[11]);
        state[12] = xor128(state[12], snapshot[12]);
        state[13] = xor128(state[13], snapshot[13]);
        state[14] = xor128(state[14], snapshot[14]);
    }
}



/* --- 静态常量查找表 --- */
#ifdef ENABLE_GARNET_512_W512
static const uint64_t IV_COUNTER_512_W512[25][2] = {
    {0x782b1463e5b448c2, 0xde30d1b89ddaa011}, // [0]
    {0xd92183bde01eaf39, 0x634ee0b3bf88f484}, // [1]
    {0xf8fb07c0f0d96f62, 0x149d3ee603859d70}, // [2]
    {0x4c92e211237707ef, 0xf48dce216aa60cac}, // [3]
    {0xd2543469c3db12b7, 0x7396d12e4c6dbbb7}, // [4]
    {0x6d127c0b76361e62, 0x8a6a75f1e6bbc959}, // [5]
    {0x833bfb64f3bce994, 0x530cf1efa5661a70}, // [6]
    {0x0e267b18c6f6db4c, 0xb0389a5d5e9c9970}, // [7]
    {0x05239d1166b906cb, 0x67117fbd723084d2}, // [8]
    {0xe907c5dd6ca25474, 0xcbbac776493d5c27}, // [9]
    {0x6d0064b8f5707068, 0xaff5e27600e5bc3f}, // [10]
    {0xc58584622d4b401e, 0x9b1ac6e440a1ef11}, // [11]
    {0x42e77c2f23ac28fb, 0x92918651c356f4b8}, // [12]
    {0x09e68e582255f277, 0x7e5c987ec3973036}, // [13]
    {0x87947a0718e95f65, 0x8b07f08ed4e24d6c}, // [14]
    {0xf742ee05141a3f41, 0xfa7f68f3b89fc0c7}, // [15]
    {0x0000000000000000, 0x0000000000000000}, // [16]
    {0x0000000000000000, 0x0000000000000000}, // [17]
    {0x0000000000000000, 0x0000000000000000}, // [18]
    {0x0000000000000000, 0x0000000000000000}, // [19]
    {0x0000000000000000, 0x0000000000000000}, // [20]
    {0x0000000000000000, 0x0000000000000000}, // [21]
    {0x0000000000000000, 0x0000000000000000}, // [22]
    {0x0000000000000000, 0x0000000000000000}, // [23]
    {0x0000000000000000, 0x0000000000000000} // [24]
};

#endif
#ifdef ENABLE_GARNET_512_W640
static const uint64_t IV_COUNTER_512_W640[25][2] = {
    {0x311cceecf76770f1, 0xffdfea80e8157eb4}, // [0]
    {0xa9af0b6d22d121bd, 0x486dae24482aa00a}, // [1]
    {0x9f2fe6c33ab00be5, 0x306eda4994baba9c}, // [2]
    {0x190a811b2f39580e, 0x5c2173082db5ddea}, // [3]
    {0xf6818434230a38a3, 0xc4f59e2ab098316c}, // [4]
    {0xb2f34e3108b93899, 0x8452307e9b13b8ae}, // [5]
    {0x8cd00f5e6f965b39, 0xd6559e703c0ffad4}, // [6]
    {0xb9bc125c20abb2a6, 0xf4139d666be07b74}, // [7]
    {0xb9e97c07cb92a584, 0xf1c65f707fb1d7a7}, // [8]
    {0x0c8649adbb445f7b, 0xa217af9dc3b9c169}, // [9]
    {0xe33cd10720739e0a, 0x0dc0cdb2822afe55}, // [10]
    {0x728e9fc267980396, 0xe060d07351d77ec8}, // [11]
    {0xe900478b90718d8f, 0x8cc7e03f0c09af90}, // [12]
    {0x533a33dac888ed08, 0x80dd4e1f759fa6b9}, // [13]
    {0xd626dbde1d3bbe21, 0x454bf20854edf819}, // [14]
    {0x1a9fe56ae1482d24, 0xa8dd28acb21d7197}, // [15]
    {0x0000000000000000, 0x0000000000000000}, // [16]
    {0x0000000000000000, 0x0000000000000000}, // [17]
    {0x0000000000000000, 0x0000000000000000}, // [18]
    {0x0000000000000000, 0x0000000000000000}, // [19]
    {0x0000000000000000, 0x0000000000000000}, // [20]
    {0x0000000000000000, 0x0000000000000000}, // [21]
    {0x0000000000000000, 0x0000000000000000}, // [22]
    {0x0000000000000000, 0x0000000000000000}, // [23]
    {0x0000000000000000, 0x0000000000000000} // [24]
};

#endif
#ifdef ENABLE_GARNET_512_W768
static const uint64_t IV_COUNTER_512_W768[25][2] = {
    {0x2ebf3abde3fb6116, 0xbaa89618b47c4eee}, // [0]
    {0x077ad97232ad617b, 0xbe0b0c92abdb3cda}, // [1]
    {0x5950cdfba838e796, 0x7e9b096bd65a5249}, // [2]
    {0xaf4541046e765fac, 0x83b61edc223526ee}, // [3]
    {0x5c62cda23fe20d1e, 0xbca55cfac429c860}, // [4]
    {0x7f579965691de336, 0x05881a1f883f16ff}, // [5]
    {0x9c15f5b5b93ff854, 0xf38538686ae0eb4e}, // [6]
    {0xfd0a7691efac606a, 0xd9c35429b75fe28a}, // [7]
    {0x292b1d791af6aec8, 0xcab740a879db30cd}, // [8]
    {0xd3b38b44b3c9d1eb, 0x417579fcb695694e}, // [9]
    {0x46538867280ecd92, 0xf727a8e7871d5fb9}, // [10]
    {0xf673c20f669d63ac, 0x36b161e3e03a98e7}, // [11]
    {0x566cc32dd61c5169, 0x6eefd4f36d1bd0a0}, // [12]
    {0xb095703d0d4e2db5, 0x0e3adafb2f9ad32e}, // [13]
    {0x8a5c72a07dd7088e, 0x25ec1ece9c94d6b5}, // [14]
    {0x1e79e0199d994a74, 0x616156ebe4b971e6}, // [15]
    {0x0000000000000000, 0x0000000000000000}, // [16]
    {0x0000000000000000, 0x0000000000000000}, // [17]
    {0x0000000000000000, 0x0000000000000000}, // [18]
    {0x0000000000000000, 0x0000000000000000}, // [19]
    {0x0000000000000000, 0x0000000000000000}, // [20]
    {0x0000000000000000, 0x0000000000000000}, // [21]
    {0x0000000000000000, 0x0000000000000000}, // [22]
    {0x0000000000000000, 0x0000000000000000}, // [23]
    {0x0000000000000000, 0x0000000000000000} // [24]
};

#endif
#ifdef ENABLE_GARNET_512_W896
static const uint64_t IV_COUNTER_512_W896[25][2] = {
    {0xd1d22a3f4aa75a20, 0x1b71dce00e40b424}, // [0]
    {0x5581ac2728f164b8, 0x314be1c5b9e8a563}, // [1]
    {0x64fc5ecb7b6692e3, 0xbb81c51d35e99877}, // [2]
    {0x5f698d2be061907c, 0x58858dc92c74f6fc}, // [3]
    {0xb9d3cb972aff6aeb, 0x998e7bc1d9162e8d}, // [4]
    {0xf26c4bf35ad8fdd2, 0x89c421090dbb2912}, // [5]
    {0xaaaa25b5b302ff3a, 0x28c09b38e704db42}, // [6]
    {0x0ea9d84402f9ac85, 0x1d66f67cad5dd335}, // [7]
    {0x9d3b80acfdced1d2, 0x2021b612913c9fd9}, // [8]
    {0xbe44220a3cce4bca, 0xd64db88cd67650bd}, // [9]
    {0x0862a4e1ee82c2d0, 0x56ee5b87799345f2}, // [10]
    {0x181c6b55876bed3a, 0xe438c2932439ce59}, // [11]
    {0xe1bdfaa580c622d7, 0x92f2db933348b7b1}, // [12]
    {0x93ac5cb01862ed75, 0xcf876eb66883cf9b}, // [13]
    {0x5b80980b7654ae96, 0x9ac1852305b140af}, // [14]
    {0x9b89523fa05230c5, 0x246f7e7391080492}, // [15]
    {0x0000000000000000, 0x0000000000000000}, // [16]
    {0x0000000000000000, 0x0000000000000000}, // [17]
    {0x0000000000000000, 0x0000000000000000}, // [18]
    {0x0000000000000000, 0x0000000000000000}, // [19]
    {0x0000000000000000, 0x0000000000000000}, // [20]
    {0x0000000000000000, 0x0000000000000000}, // [21]
    {0x0000000000000000, 0x0000000000000000}, // [22]
    {0x0000000000000000, 0x0000000000000000}, // [23]
    {0x0000000000000000, 0x0000000000000000} // [24]
};

#endif
#ifdef ENABLE_GARNET_512_W1024
static const uint64_t IV_COUNTER_512_W1024[25][2] = {
    {0xce4f49d38fe0830a, 0xa4b8433d99d491ed}, // [0]
    {0x4ca1daedfbff54fa, 0x73317b70d2b67ab3}, // [1]
    {0xa70e9223fbb3de14, 0xb9dbff2ff29f4466}, // [2]
    {0x43cd103aecc958c5, 0x01df8af21eceedf5}, // [3]
    {0x4d165d1cd86cc8d4, 0x6a32489ae800f834}, // [4]
    {0x05f9522bf88d6a96, 0x2224c4159c4951cc}, // [5]
    {0xa1d714b1ac3c0e6c, 0x6db6b4e6b6337716}, // [6]
    {0x661d5963f16a276a, 0x681866d4c5ca710d}, // [7]
    {0x57133677381d8b01, 0xf68990ae9cf6618e}, // [8]
    {0x4ce6b5a186f5b5ce, 0xfee8f188dc038a49}, // [9]
    {0x5e9cf253dcd91cfd, 0x577a951517e7a95a}, // [10]
    {0x92de402ccb02f387, 0x40dfc12faa2d68d5}, // [11]
    {0xab13fd4cbb26505d, 0xd82968bd47091988}, // [12]
    {0xf72e1a744a39c552, 0x5d459579a983f285}, // [13]
    {0x3a8a941b569abc22, 0xcae4079778f0af79}, // [14]
    {0x6fe36fc7a21df058, 0xe45f3a0f93bdfaa6}, // [15]
    {0x0000000000000000, 0x0000000000000000}, // [16]
    {0x0000000000000000, 0x0000000000000000}, // [17]
    {0x0000000000000000, 0x0000000000000000}, // [18]
    {0x0000000000000000, 0x0000000000000000}, // [19]
    {0x0000000000000000, 0x0000000000000000}, // [20]
    {0x0000000000000000, 0x0000000000000000}, // [21]
    {0x0000000000000000, 0x0000000000000000}, // [22]
    {0x0000000000000000, 0x0000000000000000}, // [23]
    {0x0000000000000000, 0x0000000000000000} // [24]
};

#endif
#ifdef ENABLE_GARNET_768_W512
static const uint64_t IV_COUNTER_768_W512[25][2] = {
    {0xa81adffbd7e4ecc5, 0x900e7220fbc0a2a9}, // [0]
    {0x452112cbd38f687b, 0xb2c6cf5cb985a343}, // [1]
    {0x6f32196521006231, 0x02532ef420b221ba}, // [2]
    {0x554af4c708f01b5b, 0x8c0aa10a77562b54}, // [3]
    {0x253b2aa5752073ee, 0x3d9b8469af4e2f62}, // [4]
    {0xfb759b1e991915e9, 0x9cc56b68c0bf330d}, // [5]
    {0x1944e4f280f0f9d6, 0x87d6ad22e0991acf}, // [6]
    {0x4d6c7fdeb90a1d16, 0x915e00c3ed564903}, // [7]
    {0x7d345c648773c306, 0xba2bc2c6d97c6bd8}, // [8]
    {0x0fc33894dffdd793, 0x08b97165f30fa389}, // [9]
    {0xa75d9c9cbc8de0df, 0x39051f12faaa087e}, // [10]
    {0x8d7bea10122be937, 0xc4eeddfb4281a22e}, // [11]
    {0x9b6b038a7b8787bc, 0x7c1e61fd2cb3f569}, // [12]
    {0xdd9fc604e7f3f995, 0xffc1cf7b127b4c8d}, // [13]
    {0x2abeb5fa34dec371, 0x1ab40f1cc68e821d}, // [14]
    {0x3b29e17602a41b9f, 0xf69fd9975760832e}, // [15]
    {0x0000000000000000, 0x0000000000000000}, // [16]
    {0x0000000000000000, 0x0000000000000000}, // [17]
    {0x0000000000000000, 0x0000000000000000}, // [18]
    {0x0000000000000000, 0x0000000000000000}, // [19]
    {0x0000000000000000, 0x0000000000000000}, // [20]
    {0x0000000000000000, 0x0000000000000000}, // [21]
    {0x0000000000000000, 0x0000000000000000}, // [22]
    {0x0000000000000000, 0x0000000000000000}, // [23]
    {0x0000000000000000, 0x0000000000000000} // [24]
};

#endif
#ifdef ENABLE_GARNET_1024_W1024
static const uint64_t IV_COUNTER_1024_W1024[25][2] = {
    {0x2f9c60438447337c, 0x7b606eb90eb13be1}, // [0]
    {0x5c8b35252fd47075, 0xcc383dd54fad8f74}, // [1]
    {0x8ef72b1c11a9ffd8, 0xb086339b79e5d402}, // [2]
    {0xaa3dea54ab977c2b, 0xdc1332b0290557d1}, // [3]
    {0x22f2588608509ece, 0x8e25a75b9a54d5e4}, // [4]
    {0x49beb9b3aa7680c4, 0xc8b7f3ec1dda38e1}, // [5]
    {0x1a4fa316d91179e7, 0xe2f3e094c92135bd}, // [6]
    {0xea826c8dc8024d4e, 0x55155da622eff44e}, // [7]
    {0x56dd5dc7f8421a81, 0xe0b6ffbd2e734c49}, // [8]
    {0x718d0c4c967a51b2, 0x450e779857e3ee0c}, // [9]
    {0xd827d55ff242a2c7, 0x6174eb58e2118588}, // [10]
    {0xef21e729a9306142, 0x5c55ff32509c6f29}, // [11]
    {0xdf683a26d28430b4, 0x4d39d0802c0674bb}, // [12]
    {0xd97ffecf650cb30a, 0xb7d4d81d7cd63692}, // [13]
    {0x6bca4edac10b3761, 0xa1df53cdc4a2b9a0}, // [14]
    {0xe1617ccf3bccd8db, 0xf0f32dd9b7e58680}, // [15]
    {0x774c4dbc7f30b06a, 0xb609838db409e039}, // [16]
    {0x38dd26c0c733cf45, 0x1a0fbc9a3cfaac82}, // [17]
    {0xb76f501a98ced1ac, 0x8d71f502e15abeb1}, // [18]
    {0xc7171e4db0f29699, 0x2ddb055195b4128b}, // [19]
    {0x1fe5e91f886753af, 0x5de65b7f5ea7f37b}, // [20]
    {0x1dde2f791b656113, 0x68e8d79f7af25c67}, // [21]
    {0xe82b4a980fbc409e, 0xecee3c6b5a784d3c}, // [22]
    {0xfb1cd1b91d9d1163, 0xddc0d5ade32561df}, // [23]
    {0x07eed9e25348df8e, 0xa2e6f353b5ac58e7} // [24]
};

#endif
#ifdef ENABLE_GARNET_1024_W1152
static const uint64_t IV_COUNTER_1024_W1152[25][2] = {
    {0x54a882018bc0768d, 0x2111e3e6172cc78f}, // [0]
    {0x7a6e7cda7b35a335, 0x12b6ac326eb9df1f}, // [1]
    {0xb6be50328b430676, 0x59424a59e3edf07b}, // [2]
    {0x6c20586c34f48e49, 0x16447ce951a8bc69}, // [3]
    {0xee0e6e342815fa09, 0x687cc1be004ee710}, // [4]
    {0x2cf36d30bf001cfa, 0x12d27c6f94315128}, // [5]
    {0x99484e011eae57a1, 0x822f89fcb3af1859}, // [6]
    {0x2bf91265919f6cb3, 0x2ef54cc0b5d0e2b6}, // [7]
    {0x021b393f985652aa, 0x2a4c79530a1d8e8f}, // [8]
    {0x21f5176e3a839c20, 0x7d24afec3e8aa95f}, // [9]
    {0x6d0de719d01fef19, 0xd373131d3a13fdff}, // [10]
    {0x3a79d769dec43986, 0x674cfbf7c825e8e6}, // [11]
    {0x7e4d59ab70302bd7, 0x799956af18b8b77e}, // [12]
    {0xebd1314e19cf2acd, 0x4147945fda8bf005}, // [13]
    {0xa376a16af86d0a4a, 0xb1951830036ae816}, // [14]
    {0x7c7b18e108480050, 0x9111c5e9fd2130ec}, // [15]
    {0xdde92e20673c66c4, 0x349d7f6f52a247b2}, // [16]
    {0xa995ca86cc1a59a5, 0xc036eada7b580e86}, // [17]
    {0xd7950ccfcc95ffde, 0x6014137a4920d2e8}, // [18]
    {0x935d22d172275943, 0x44ef32a937e86532}, // [19]
    {0x5618541c8c90a9e2, 0x699ef8b03fff33ae}, // [20]
    {0xaf21bf71dd63bf16, 0x40faee0e931f74ec}, // [21]
    {0x07938b80b53f3bcf, 0x02cff2b0ddcd5857}, // [22]
    {0x0b0130d9d6ae8772, 0x846efbd14df741d9}, // [23]
    {0x2dec5c87e5edc098, 0x5628dd4322561df8} // [24]
};

#endif
#ifdef ENABLE_GARNET_1024A_SP_DM
static const uint64_t IV_COUNTER_1024A_SP_DM[25][2] = {
    {0x6d6794b2b44ce079, 0xae3e63c412c399c7}, // [0]
    {0x3313b546b6b97c6a, 0x4ba73a9694578b7b}, // [1]
    {0x1ffe7cde7b97f757, 0xf1732c187b567448}, // [2]
    {0x3ca91ae242531b1b, 0xf306671bb4c3825d}, // [3]
    {0x7f93197cdf02b501, 0xfb30f2348484e130}, // [4]
    {0xea2ca2425a036ee4, 0xfabc1c4d4fc43e64}, // [5]
    {0x11d429de5a6f596c, 0x706de8c85e5c85ef}, // [6]
    {0x0c62de986f621216, 0xf768093617b362bd}, // [7]
    {0xe9bb5cabecfdc033, 0xcb8d620f0de9ab02}, // [8]
    {0xf3786fd62accae7f, 0xda3f7bac0b8a4288}, // [9]
    {0x5025d51190f4aef1, 0x29c38914c6d95522}, // [10]
    {0xfa559a587be6b290, 0xd35f234f7bb89d5c}, // [11]
    {0x0e7454120469cf3c, 0x9b5e142fb1b033fb}, // [12]
    {0xe259137329632770, 0xc5b11c6410d4ed09}, // [13]
    {0x33616a45fa80ee0b, 0x9cf6fefd3c9cf33b}, // [14]
    {0x194ea6997ef891f9, 0xf90e95fde810f135}, // [15]
    {0x0000000000000000, 0x0000000000000000}, // [16]
    {0x0000000000000000, 0x0000000000000000}, // [17]
    {0x0000000000000000, 0x0000000000000000}, // [18]
    {0x0000000000000000, 0x0000000000000000}, // [19]
    {0x0000000000000000, 0x0000000000000000}, // [20]
    {0x0000000000000000, 0x0000000000000000}, // [21]
    {0x0000000000000000, 0x0000000000000000}, // [22]
    {0x0000000000000000, 0x0000000000000000}, // [23]
    {0x0000000000000000, 0x0000000000000000} // [24]
};

#endif
#ifdef ENABLE_GARNET_1024_W2048_DM
static const uint64_t IV_COUNTER_1024_W2048_DM[25][2] = {
    {0xaec9cc6d006bfbd0, 0xba0312660dff27cb}, // [0]
    {0xae9dbc64e57ab67a, 0x09325fc74b7ef7b1}, // [1]
    {0x73b305015e8b5ea5, 0xc3874bb84771eac9}, // [2]
    {0x8c259552bead32ae, 0xf0c3b8b20074f0d9}, // [3]
    {0x597e89bb4afd3824, 0x7761e834210eebd0}, // [4]
    {0xad10ec363db9e7ef, 0xfd36609779db5ec8}, // [5]
    {0xbcf4e03ccd48827e, 0x91112132cd1a9981}, // [6]
    {0x04e885aa4a913cfc, 0xbac4eb6b4fe8a2fb}, // [7]
    {0x17d7dbf0cbbbd640, 0xa40702ab8a3c22d8}, // [8]
    {0xd177ee2674ba38fe, 0x49f5a370b52f8cac}, // [9]
    {0xf342493a2b467a2d, 0xd14773cee5647ab0}, // [10]
    {0xa9fec6ef5d833033, 0x18103fb7ad5e4199}, // [11]
    {0x58f9953804866206, 0x5d289afe47228aea}, // [12]
    {0xf4614b56fa9566c7, 0xfbbe2145b32433e6}, // [13]
    {0xe524e3946369694b, 0x22dbe3b7620c8d8c}, // [14]
    {0xb1b8fcd1d792fd17, 0x4c92d6e1065617df}, // [15]
    {0x85e01bb25bc8f219, 0x99fb962829d16bae}, // [16]
    {0xbdfff2941b1fd18d, 0x99265ce7c16f7a45}, // [17]
    {0x8740116fbd01b745, 0x79c998665f397765}, // [18]
    {0x2bc7341dbd58f30a, 0x87e9942df05a0de3}, // [19]
    {0xcadc9c0acc6d0ad3, 0x00482b139c21815e}, // [20]
    {0x7311876495cb404e, 0xb07634315e21df2c}, // [21]
    {0x33e26bb1071dbd32, 0x1d5ddc965d8e0b56}, // [22]
    {0x14b41d6e188b5b4b, 0xe4958c00e442220f}, // [23]
    {0x1f9ff3036a2df312, 0xd6007c4f9b77b27c} // [24]
};

#endif

GARNET_FAST_FUNC
void initialize_state_st(struct uint128_t state[25], uint64_t digest_size, uint64_t counter)
{
    (void)digest_size;
    switch (counter) {
#ifdef ENABLE_GARNET_512_W512
        case COUNTER_512_W512:
            memcpy(state, IV_COUNTER_512_W512, sizeof(IV_COUNTER_512_W512));
            break;
#endif
#ifdef ENABLE_GARNET_512_W640
        case COUNTER_512_W640:
            memcpy(state, IV_COUNTER_512_W640, sizeof(IV_COUNTER_512_W640));
            break;
#endif
#ifdef ENABLE_GARNET_512_W768
        case COUNTER_512_W768:
            memcpy(state, IV_COUNTER_512_W768, sizeof(IV_COUNTER_512_W768));
            break;
#endif
#ifdef ENABLE_GARNET_512_W896
        case COUNTER_512_W896:
            memcpy(state, IV_COUNTER_512_W896, sizeof(IV_COUNTER_512_W896));
            break;
#endif
#ifdef ENABLE_GARNET_512_W1024
        case COUNTER_512_W1024:
            memcpy(state, IV_COUNTER_512_W1024, sizeof(IV_COUNTER_512_W1024));
            break;
#endif
#ifdef ENABLE_GARNET_768_W512
        case COUNTER_768_W512:
            memcpy(state, IV_COUNTER_768_W512, sizeof(IV_COUNTER_768_W512));
            break;
#endif
#ifdef ENABLE_GARNET_1024_W1024
        case COUNTER_1024_W1024:
            memcpy(state, IV_COUNTER_1024_W1024, sizeof(IV_COUNTER_1024_W1024));
            break;
#endif
#ifdef ENABLE_GARNET_1024_W1152
        case COUNTER_1024_W1152:
            memcpy(state, IV_COUNTER_1024_W1152, sizeof(IV_COUNTER_1024_W1152));
            break;
#endif
#ifdef ENABLE_GARNET_1024A_SP_DM
        case COUNTER_1024A_SP_DM:
            memcpy(state, IV_COUNTER_1024A_SP_DM, sizeof(IV_COUNTER_1024A_SP_DM));
            break;
#endif
#ifdef ENABLE_GARNET_1024_W2048_DM
        case COUNTER_1024_W2048_DM:
            memcpy(state, IV_COUNTER_1024_W2048_DM, sizeof(IV_COUNTER_1024_W2048_DM));
            break;
#endif
        default:
            memset(state, 0, 25 * sizeof(struct uint128_t));
            break;
    }
}
// ---------------------------------------------------------
// 辅助函数：根据 Counter 精确推断输出摘要的长度
// (支持宏裁剪，未启用的模式将被编译器优化掉)
// ---------------------------------------------------------
static inline uint32_t get_digest_bits_from_counter(uint64_t counter) 
{
uint32_t target_bits = 1024; // Default

    switch (counter) {
        
#if defined(ENABLE_GARNET_512_W512) || defined(ENABLE_GARNET_512_W640) || \
    defined(ENABLE_GARNET_512_W768) || defined(ENABLE_GARNET_512_W896) || \
    defined(ENABLE_GARNET_512_W1024)
        case COUNTER_512_W512:
        case COUNTER_512_W640:
        case COUNTER_512_W768:
        case COUNTER_512_W896:
        case COUNTER_512_W1024:
            target_bits = 512;
            break;
#endif

#ifdef ENABLE_GARNET_768_W512
        case COUNTER_768_W512:
            target_bits = 768;
            break;
#endif

#if defined(ENABLE_GARNET_1024_W1024) || defined(ENABLE_GARNET_1024_W1152) || \
    defined(ENABLE_GARNET_1024A_SP_DM)
        case COUNTER_1024_W1024:
        case COUNTER_1024_W1152:
        case COUNTER_1024A_SP_DM:
            target_bits = 1024;
            break;
#endif
    }
    
    return target_bits;
}

/**
 * @brief 32位 ARM 优化哈希引擎 
 */
GARNET_FAST_FUNC
void hash_garnet_universal_st(const u8* message, u64 mlength, u8* digest, 
                              struct uint128_t state[25], uint64_t counter)
{
    int i;
    uint32_t j;
    struct uint128_t msg[16];      // 必须为 16
    struct uint128_t snapshot[25]; // 用于 Davies-Meyer 前馈快照
    struct uint128_t zero_key = {0}, one_key = {0}, two_key = {0};

    // --- 1. 参数解析 (48-bit Counter) ---
    uint8_t  is_dm        = GET_MODE(counter);           
    uint32_t rate_bits    = GET_RATE_BITS(counter);      
    uint32_t rate_bytes   = rate_bits >> 3;              
    uint32_t rate_blocks  = rate_bits >> 7;              
    uint8_t  abs_rounds   = GET_ABS_RNDS(counter);       
    uint8_t  mid_rounds   = GET_MID_RNDS(counter);       
    uint8_t  sqz_rounds   = GET_SQZ_RNDS(counter);       
    int      is_5x5       = IS_5X5_MATRIX(counter);
    int      is_flagship  = IS_FLAGSHIP_5x5(counter);

    // 字节总数
    u64 byte_num = (mlength % 8 == 0) ? (mlength >> 3) : ((mlength >> 3) + 1);

        switch (counter) {
        // Garnet-512 家族
#if defined(ENABLE_GARNET_512_W512) || defined(ENABLE_GARNET_512_W640) || \
    defined(ENABLE_GARNET_512_W768) || defined(ENABLE_GARNET_512_W896) || \
    defined(ENABLE_GARNET_512_W1024)
        case COUNTER_512_W512:
        case COUNTER_512_W640:
        case COUNTER_512_W768:
        case COUNTER_512_W896:
        case COUNTER_512_W1024:
            one_key.u.v[0] = 1; two_key.u.v[0] = 2;
            break;
#endif
        // Garnet-768 家族
#ifdef ENABLE_GARNET_768_W512
        case COUNTER_768_W512:
            one_key.u.v[0] = 3; two_key.u.v[0] = 4;
            break;
#endif
        // Garnet-1024 (5x5) 家族
#if defined(ENABLE_GARNET_1024_W1024) || defined(ENABLE_GARNET_1024_W1152)
        case COUNTER_1024_W1024:
        case COUNTER_1024_W1152:
            one_key.u.v[0] = 5; two_key.u.v[0] = 6;
            break;
#endif

#ifdef ENABLE_GARNET_1024_W2048_DM
        case COUNTER_1024_W2048_DM:
            one_key.u.v[0] = 5; two_key.u.v[0] = 6;
            break;
#endif
        // Garnet-1024a (Sponge-DM 4x4)
#ifdef ENABLE_GARNET_1024A_SP_DM
        case COUNTER_1024A_SP_DM:
            one_key.u.v[0] = 7; two_key.u.v[0] = 8;
            break;
#endif
    }

#if GARNET_DEBUG
    printf("\r\n[DEBUG] counter=0x%012llx | mode=%s\n", (unsigned long long)counter, is_dm ? "DM" : "Sponge");
    printf("[DEBUG] is_5x5=%d | rate_bits=%u | rate_bytes=%u | rate_blocks=%u\n", is_5x5, rate_bits, rate_bytes, rate_blocks);
    printf("[DEBUG] rounds: abs=%u | mid=%u | sqz=%u\n", abs_rounds, mid_rounds, sqz_rounds);
    printf("[DEBUG] mlength=%llu | byte_num=%llu\n", (unsigned long long)mlength, (unsigned long long)byte_num);
#endif
    // ==========================================
    // 阶段一：消息吸收主循环
    // ==========================================
    u64 processed = 0;
    while ((processed + rate_bytes) <= byte_num)
    {
        for (j = 0; j < rate_blocks; j++) {
            memcpy(&msg[j], (u8*)message + processed + (j << 4), 16);
        }
        if (is_dm) {
            // 备份 Capacity (旗舰版 9格，4x4 全量)
            if (is_flagship) {
                const uint8_t c_idx[9] = {1, 3, 5, 9, 15, 16, 19, 21, 23};
                for (int k = 0; k < 9; k++) snapshot[k] = state[c_idx[k]];
            } else {
                for (int k = 0; k < 16; k++) snapshot[k] = state[k];
            }
        }
        // --- 置换前吸收 ---
        absorb_dynamic_msg(state, msg, is_5x5, (int)rate_blocks, is_dm, counter);
        
        for (j = 0; j < abs_rounds; j++) {
            if (is_5x5) permutation_p_5x5_st(state, &zero_key, j);
            else        permutation_p_4x4_st(state, &zero_key, j);
        }

        if (is_dm) {
            absorb_dynamic_msg(state, msg, is_5x5, (int)rate_blocks, is_dm, counter);
            absorb_dynamic_capacity(state, snapshot, is_5x5);
        }
        processed += rate_bytes;
    }

    // ==========================================
    // 阶段二：填充与尾块处理
    // ==========================================
    for (j = 0; j < 16; j++) zero128(&msg[j]);
    j = 0;
    while ((processed + 16) <= byte_num) {
        memcpy(&msg[j++], (u8*)message + processed, 16);
        processed += 16;
    }
    if (processed < byte_num) {
        u8 lastblock[16] = {0};
        memcpy(lastblock, message + processed, (size_t)(byte_num - processed) + 1); 
        memcpy(&msg[j], lastblock, 16);
    } else if ((mlength % 8) == 0) {
        msg[j].u.n[0] = 0x80;
    }

#if GARNET_DEBUG
    printf("[DEBUG] main_loop_iterations=%d | tail_start_i=%llu\n", (int)(processed / (rate_bytes?rate_bytes:1)), (unsigned long long)processed);
    printf("[DEBUG] --- tail msg dump (rate_blocks=%u) ---\n", (unsigned int)rate_blocks);
    for (uint32_t k = 0; k < rate_blocks; k++) {
        printf("[DEBUG]   msg[%u]: %016llx %016llx\n", k, (unsigned long long)msg[k].u.v[1], (unsigned long long)msg[k].u.v[0]);
    }
#endif

    if (is_dm) {
        if (is_flagship) {
            const uint8_t c_idx[9] = {1, 3, 5, 9, 15, 16, 19, 21, 23};
            for (int k = 0; k < 9; k++) snapshot[k] = state[c_idx[k]];
        } else for (int k = 0; k < 16; k++) snapshot[k] = state[k];
    }
    absorb_dynamic_msg(state, msg, is_5x5, (int)rate_blocks, is_dm, counter);
    for (j = 0; j < abs_rounds; j++) {
        if (is_5x5) permutation_p_5x5_st(state, &zero_key, j);
        else        permutation_p_4x4_st(state, &zero_key, j);
    }
    if (is_dm) {
        absorb_dynamic_msg(state, msg, is_5x5, (int)rate_blocks, is_dm, counter); 
        absorb_dynamic_capacity(state, snapshot, is_5x5);
    }
    // --- 注入长度与收尾 ---
    absorb_dynamic_length(state, mlength, is_5x5);

    for (i = 0; i < (int)mid_rounds; i++) {
        if (is_5x5) permutation_p_5x5_st(state, &one_key, i);
        else        permutation_p_4x4_st(state, &one_key, i);
    }
    for (i = 0; i < (int)sqz_rounds; i++) {
        if (is_5x5) permutation_p_5x5_st(state, &two_key, i);
        else        permutation_p_4x4_st(state, &two_key, i);
    }

    extract_digest_dynamic(digest, state, is_5x5, get_digest_bits_from_counter(counter));

#if GARNET_DEBUG
    printf("[DEBUG] === Garnet Engine Finished ===\r\n");
#endif
}
/**
 * @brief 标准接口实现
 */
#include <string.h>
#include "garnet_config.h"

/**
 * @brief 以二进制和十六进制混合格式打印消息
 * @param label  日志标签
 * @param msg    缓冲区指针
 * @param bit_len 原始比特长度
 */
void log_msg_binary(const char* label, const unsigned char *msg, unsigned long long bit_len) {
    // 计算 pad_message 之后受影响的字节数
    // 逻辑：如果 bit_len = 3, 则处理 1 字节；如果 bit_len = 8, 则处理 2 字节（因为补了 0x80）
    size_t byte_len = (bit_len / 8) + 1;

    printf("\n--- %s ---\n", label);
    printf("Original Bit Len: %llu, Bytes to display: %zu\n", bit_len, byte_len);
    printf("Addr | Hex | Binary (MSB -> LSB)\n");
    printf("----------------------------------\n");

    for (size_t i = 0; i < byte_len; i++) {
        printf("%04zu | %02X  | ", i, msg[i]);
        for (int b = 7; b >= 0; b--) {
            printf("%d", (msg[i] >> b) & 1);
            if (b == 4) printf(" "); // 增加 4 位分隔，方便阅读
        }
        // 在该字节行末尾标注是否是最后一个字节
        if (i == (bit_len / 8)) printf(" <--- Tail");
        printf("\n");
    }
    printf("----------------------------------\n\n");
}

/**
 * @brief 字节序适配层：全缓冲区翻转
 */
static void garnet_port_reverse_buffer(unsigned char *msg, size_t byte_len) {
    if (byte_len <= 1) return;
    size_t i = 0, j = byte_len - 1;
    while (i < j) {
        unsigned char t = msg[i];
        msg[i] = msg[j];
        msg[j] = t;
        i++; j--;
    }
}
/**
 * @brief 修正后的 ICCS 标准接口
 * 解决了静态宏锁定逻辑的问题，确保 512/768/1024 真正运行各自的算法
 */
int CryptHash(int digest_len_bits,  unsigned char *msg, 
              unsigned long long msg_len_bits, unsigned char *digest) 
{



    // 1. 计算当前消息占用的总字节数
    size_t byte_len = (size_t)((msg_len_bits + 7) / 8);
    // 如果是整字节，pad_message 通常会多占一个字节(0x80)，所以这里多算一个字节的安全位
    if (msg_len_bits % 8 == 0) byte_len += 1;

    // 2. 
    // 此时 bitlen=3 会在 msg[0] 产生 0xB0 (如果是单字节消息)
    pad_message(msg, msg_len_bits);

    // 3. 
    // 如果消息是 20 字节，原本在 msg[0] 的数据和填充位会被搬到 msg[19]
    garnet_port_reverse_buffer(msg, byte_len);

    // 打印 Log 观察转换后的内存，看填充位是不是跑到“物理开头”去了
    log_msg_binary("MSG_AFTER_PORTING", msg, msg_len_bits);

    uint64_t counter;
    // --- 1. 动态映射 (必须基于参数 digest_len_bits) ---
    // 不要在这里使用 #if DIGEST_BIT_LENGTH，除非你只想生成单一算法的库
    switch (digest_len_bits) {
        case 512:
            counter = ACTIVE_COUNTER_512;
            break;
        case 768:
            counter = ACTIVE_COUNTER_768;
            break;
        case 1024:
            counter = ACTIVE_COUNTER_1024;
            break;
        default:
            return -1; // 不支持的摘要长度
    }

    // --- 2. 初始化状态矩阵 ---
    // 在栈上分配 400 字节，符合 ARM Cortex-M 栈深度建议
    struct uint128_t state[25] __attribute__((aligned(16)));;
    
    // 显式清零。在 ARM GCC -O3 下，memset 会被优化为极快的 STM 指令
    memset(state, 0, sizeof(state));
    
    // 初始化 IV 和常数 (传入正确的 counter)
    initialize_state_st(state, (uint32_t)digest_len_bits, counter);

    // --- 3. 执行核心哈希运算 ---
    // 内部已优化为零拷贝处理
    hash_garnet_universal_st(msg, msg_len_bits, digest, state, counter);

    return 0;
}