/*
 * This file provides the command-line test harness for the optimized CHIME-1024 implementation.
 * It reads the reference-generated test vectors, runs the optimized hash function, and checks the computed digests against the expected outputs.
 *
 * 本文件为 CHIME-1024 的优化实现提供命令行测试驱动。
 * 它读取参考实现生成的测试向量，运行优化后的哈希函数，并将计算得到的摘要与期望输出进行核对。
 */

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include "chime1024.hpp"

/*
 * Converts a byte vector into an uppercase hexadecimal string.
 * Inputs:
 *     v: byte vector to convert
 * Outputs:
 *     return value: hexadecimal string representation
 *
 * 将字节向量转换为大写十六进制字符串。
 * 输入：
 *     v：待转换的字节向量
 * 输出：
 *     返回值：十六进制字符串表示
 */
static std::string to_hex(const std::vector<uint8_t>& v) {
    static const char* kHex = "0123456789ABCDEF";
    std::string s;
    s.reserve(v.size() * 2);
    for (uint8_t b : v) {
        s.push_back(kHex[b >> 4]);
        s.push_back(kHex[b & 0x0F]);
    }
    return s;
}

/*
 * Stores one parsed test vector entry.
 * 存储一条解析后的测试向量记录。
 */
struct TestVector {
    uint64_t msg_len_bits = 0;
    std::vector<uint8_t> msg_bytes;

    uint64_t dst_len_bits = 0;
    std::vector<uint8_t> dst_bytes;
};

/*
 * Represents the hashing function signature used by the test harness.
 * 表示测试驱动中使用的哈希函数签名。
 */
using HashFn = int (*)(const uint8_t* msg, size_t len_bits, uint8_t* digest);

/*
 * Runs all loaded test vectors against a hash function and reports mismatches.
 * Inputs:
 *     vectors: parsed test vector list
 *     hash_fn: hash function to execute
 *     test_non_byte_aligned: whether to include non-byte-aligned messages
 *     expected_digest_bytes: digest size expected by the harness
 * Outputs:
 *     return value: true if all tested vectors pass, false otherwise
 *
 * 使用给定哈希函数运行所有已加载测试向量并报告不匹配项。
 * 输入：
 *     vectors：解析后的测试向量列表
 *     hash_fn：待执行的哈希函数
 *     test_non_byte_aligned：是否包含非字节对齐消息
 *     expected_digest_bytes：测试框架期望的摘要字节数
 * 输出：
 *     返回值：若所有测试向量通过则返回 true，否则返回 false
 */
static bool run_test_vectors(const std::vector<TestVector>& vectors,
                             HashFn hash_fn,
                             bool test_non_byte_aligned,
                             size_t expected_digest_bytes) {
    if (hash_fn == nullptr) {
        std::printf("[FAIL] hash function is null\n");
        return false;
    }

    size_t tested = 0;
    size_t skipped = 0;

    for (size_t i = 0; i < vectors.size(); ++i) {
        const TestVector& tv = vectors[i];

        if (!test_non_byte_aligned && ((tv.msg_len_bits & 7ull) != 0ull)) {
            ++skipped;
            continue;
        }

        if ((tv.dst_len_bits & 7ull) != 0ull) {
            std::printf("[FAIL] vector %zu: Dst_Len is not byte-aligned\n", i);
            return false;
        }

        if (tv.dst_len_bits != expected_digest_bytes * 8ull) {
            std::printf("[FAIL] vector %zu: only 1024-bit outputs are supported now\n", i);
            return false;
        }

        std::vector<uint8_t> got(expected_digest_bytes, 0);

        bool ok = true;
        try {
            hash_fn(tv.msg_bytes.data(),
                    static_cast<size_t>(tv.msg_len_bits),
                    got.data());
        } catch (...) {
            ok = false;
        }

        if (!ok) {
            std::printf("[FAIL] vector %zu: hash function threw/failed\n", i);
            std::printf("  Msg_Len = %llu\n", (unsigned long long)tv.msg_len_bits);
            std::printf("  Msg     = %s\n", to_hex(tv.msg_bytes).c_str());
            return false;
        }

        if (got != tv.dst_bytes) {
            std::printf("[FAIL] vector %zu: digest mismatch\n", i);
            std::printf("  Msg_Len = %llu\n", (unsigned long long)tv.msg_len_bits);
            std::printf("  Msg     = %s\n", to_hex(tv.msg_bytes).c_str());
            std::printf("  Dst_Len = %llu\n", (unsigned long long)tv.dst_len_bits);
            std::printf("  Expect  = %s\n", to_hex(tv.dst_bytes).c_str());
            std::printf("  Got     = %s\n", to_hex(got).c_str());
            return false;
        }

        ++tested;
        //std::printf("[PASS] vector %zu\n", i);
    }

    std::printf("All %zu tested vectors passed", tested);
    if (skipped != 0) {
        std::printf(" (%zu skipped)", skipped);
    }
    std::printf(".\n");

    return true;
}

/*
 * Trims leading and trailing whitespace from a string.
 * Inputs:
 *     s: input string
 * Outputs:
 *     return value: trimmed string
 *
 * 去除字符串两端空白字符。
 * 输入：
 *     s：输入字符串
 * 输出：
 *     返回值：去除首尾空白后的字符串
 */
static std::string trim(const std::string& s) {
    size_t l = 0;
    while (l < s.size() && std::isspace(static_cast<unsigned char>(s[l]))) {
        ++l;
    }

    size_t r = s.size();
    while (r > l && std::isspace(static_cast<unsigned char>(s[r - 1]))) {
        --r;
    }

    return s.substr(l, r - l);
}

/*
 * Checks whether a string begins with a given prefix.
 * Inputs:
 *     s: input string
 *     prefix: prefix to test
 * Outputs:
 *     return value: true if s starts with prefix, otherwise false
 *
 * 检查字符串是否以给定前缀开头。
 * 输入：
 *     s：输入字符串
 *     prefix：待检查前缀
 * 输出：
 *     返回值：如果 s 以 prefix 开头则为 true，否则为 false
 */
static bool starts_with(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

/*
 * Parses a key-value line in the form "Key = Value".
 * Inputs:
 *     line: input line
 *     key: key name to match
 * Outputs:
 *     return value: parsed value string if the key matches, otherwise std::nullopt
 *
 * 解析形如 "Key = Value" 的键值行。
 * 输入：
 *     line：输入行
 *     key：要匹配的键名
 * 输出：
 *     返回值：如果键匹配则返回解析出的值字符串，否则返回 std::nullopt
 */
static std::optional<std::string> parse_key_value(const std::string& line,
                                                  const std::string& key) {
    std::string prefix = key + " =";
    if (!starts_with(line, prefix)) {
        return std::nullopt;
    }
    return trim(line.substr(prefix.size()));
}

/*
 * Converts a single hexadecimal character to its numeric value.
 * Inputs:
 *     c: hexadecimal character
 * Outputs:
 *     return value: numeric value in [0, 15], or -1 for invalid input
 *
 * 将单个十六进制字符转换为数值。
 * 输入：
 *     c：十六进制字符
 * 输出：
 *     返回值：[0, 15] 范围内的数值，非法输入返回 -1
 */
static int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

/*
 * Parses a hexadecimal string into bytes, ignoring whitespace.
 * Inputs:
 *     hex_in: hexadecimal input string
 *     out: output byte vector
 * Outputs:
 *     return value: true on success, false on invalid input
 *
 * 将十六进制字符串解析为字节，忽略空白字符。
 * 输入：
 *     hex_in：十六进制输入字符串
 *     out：输出字节向量
 * 输出：
 *     返回值：成功返回 true，输入非法返回 false
 */
static bool parse_hex_bytes(const std::string& hex_in, std::vector<uint8_t>& out) {
    std::string hex;
    hex.reserve(hex_in.size());

    for (char c : hex_in) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            hex.push_back(c);
        }
    }

    if (hex.empty()) {
        out.clear();
        return true;
    }

    if ((hex.size() & 1u) != 0u) {
        return false;
    }

    out.clear();
    out.reserve(hex.size() / 2);

    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = hex_value(hex[i]);
        int lo = hex_value(hex[i + 1]);
        if (hi < 0 || lo < 0) {
            return false;
        }
        out.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }

    return true;
}


/*
 * Parses test vectors from a text file into memory.
 * Inputs:
 *     path: test vector file path
 *     vectors: output vector list
 *     err: error message on failure
 * Outputs:
 *     return value: true on success, false on parse or I/O failure
 *
 * 从文本文件中解析测试向量并存入内存。
 * 输入：
 *     path：测试向量文件路径
 *     vectors：输出向量列表
 *     err：失败时的错误信息
 * 输出：
 *     返回值：成功返回 true，解析或 I/O 失败返回 false
 */
static bool parse_test_vectors_file(const std::string& path,
                                    std::vector<TestVector>& vectors,
                                    std::string& err) {
    std::ifstream fin(path);
    if (!fin) {
        err = "Cannot open file: " + path;
        return false;
    }

    vectors.clear();

    TestVector cur;
    bool have_msg_len = false;
    bool have_msg = false;
    bool have_dst_len = false;
    bool have_dst = false;

    auto flush_current = [&](bool at_eof) -> bool {
        if (!have_msg_len && !have_msg && !have_dst_len && !have_dst) {
            return true;
        }

        if (!(have_msg_len && have_msg && have_dst_len && have_dst)) {
            err = at_eof
                ? "Incomplete test vector at end of file."
                : "Incomplete test vector before next vector.";
            return false;
        }

        if (cur.dst_len_bits % 8 != 0) {
            err = "Dst_Len is not a multiple of 8.";
            return false;
        }

        if (cur.msg_bytes.size() * 8ull < cur.msg_len_bits) {
            err = "Msg hex string is too short for Msg_Len.";
            return false;
        }

        if (cur.dst_bytes.size() * 8ull != cur.dst_len_bits) {
            err = "Dst hex size does not match Dst_Len.";
            return false;
        }

        vectors.push_back(cur);

        cur = TestVector{};
        have_msg_len = false;
        have_msg = false;
        have_dst_len = false;
        have_dst = false;
        return true;
    };

    std::string line;
    size_t line_no = 0;

    while (std::getline(fin, line)) {
        ++line_no;
        std::string t = trim(line);

        if (t.empty()) {
            continue;
        }

        if (auto v = parse_key_value(t, "Msg_Len")) {
            if (!flush_current(false)) {
                err += " At line " + std::to_string(line_no) + ".";
                return false;
            }
            try {
                cur.msg_len_bits = std::stoull(*v);
            } catch (...) {
                err = "Invalid Msg_Len at line " + std::to_string(line_no) + ".";
                return false;
            }
            have_msg_len = true;
            continue;
        }

        if (auto v = parse_key_value(t, "Msg")) {
            if (!parse_hex_bytes(*v, cur.msg_bytes)) {
                err = "Invalid Msg hex at line " + std::to_string(line_no) + ".";
                return false;
            }
            have_msg = true;
            continue;
        }

        if (auto v = parse_key_value(t, "Dst_Len")) {
            try {
                cur.dst_len_bits = std::stoull(*v);
            } catch (...) {
                err = "Invalid Dst_Len at line " + std::to_string(line_no) + ".";
                return false;
            }
            have_dst_len = true;
            continue;
        }

        if (auto v = parse_key_value(t, "Dst")) {
            if (!parse_hex_bytes(*v, cur.dst_bytes)) {
                err = "Invalid Dst hex at line " + std::to_string(line_no) + ".";
                return false;
            }
            have_dst = true;
            continue;
        }

        err = "Unrecognized line at " + std::to_string(line_no) + ": " + t;
        return false;
    }

    if (!flush_current(true)) {
        return false;
    }

    return true;
}

/*
 * Describes one algorithm entry in the test runner.
 * 描述测试运行器中的一条算法记录。
 */
struct AlgoEntry {
    const char* name;
    HashFn fn;
    bool test_non_byte_aligned;
    const char* vector_path;
    size_t expected_digest_bytes;
};

/*
 * Runs the CHIME-1024 optimized test harness over all configured algorithms.
 * Inputs:
 *     none
 * Outputs:
 *     return value: 0 on success, 1 on parse or test failure
 *
 * 运行 CHIME-1024 优化实现的测试驱动，覆盖所有配置的算法。
 * 输入：
 *     无
 * 输出：
 *     返回值：成功返回 0，解析或测试失败返回 1
 */
int main() {
    static const AlgoEntry algos[] = {
        {
            "CHIME-1024",
            chime_1024_bit_padding,
            true,
            "../../../Test_Vector/KAT_2_12_CHIME-1024.txt",
            128
        }
    };

    for (const auto& algo : algos) {
        std::printf("============================================================\n");
        std::printf("Testing: %s\n", algo.name);
        std::printf("Vector file: %s\n", algo.vector_path);
        std::printf("============================================================\n");

        std::vector<TestVector> vectors;
        std::string err;

        if (!parse_test_vectors_file(algo.vector_path, vectors, err)) {
            std::fprintf(stderr, "Parse error in %s: %s\n", algo.vector_path, err.c_str());
            return 1;
        }

        if (!run_test_vectors(vectors, algo.fn, algo.test_non_byte_aligned, algo.expected_digest_bytes)) {
            return 1;
        }
    }

    return 0;
}