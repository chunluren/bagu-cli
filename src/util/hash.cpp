#include "util/hash.h"

#include <openssl/evp.h>

#include <array>
#include <cstdio>
#include <fstream>

namespace bagu::util {

namespace {

std::string to_hex(const unsigned char* data, size_t len) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.resize(len * 2);
    for (size_t i = 0; i < len; ++i) {
        out[i * 2]     = kHex[(data[i] >> 4) & 0xF];
        out[i * 2 + 1] = kHex[data[i] & 0xF];
    }
    return out;
}

}  // namespace

std::string sha256(std::string_view data) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return {};

    std::array<unsigned char, EVP_MAX_MD_SIZE> hash{};
    unsigned int len = 0;

    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, data.data(), data.size());
    EVP_DigestFinal_ex(ctx, hash.data(), &len);
    EVP_MD_CTX_free(ctx);

    return to_hex(hash.data(), len);
}

Result<std::string> sha256_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return make_err<std::string>(E::kFileReadFailed,
            "无法读取文件", "path=" + path.string());
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return make_err<std::string>(E::kInternal, "EVP_MD_CTX_new failed");

    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);

    std::array<char, 8192> buf{};
    while (in) {
        in.read(buf.data(), buf.size());
        std::streamsize n = in.gcount();
        if (n > 0) {
            EVP_DigestUpdate(ctx, buf.data(), static_cast<size_t>(n));
        }
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> hash{};
    unsigned int len = 0;
    EVP_DigestFinal_ex(ctx, hash.data(), &len);
    EVP_MD_CTX_free(ctx);

    return to_hex(hash.data(), len);
}

}  // namespace bagu::util
