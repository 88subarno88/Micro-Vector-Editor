#include "auth/RefreshToken.h"

#include <sodium.h>

#include <stdexcept>

namespace vec::refresh {

NewToken generate() {
    if (sodium_init() < 0) throw std::runtime_error("libsodium failed to initialise");

    unsigned char bytes[32];
    randombytes_buf(bytes, sizeof(bytes));
    char encoded[sodium_base64_ENCODED_LEN(sizeof(bytes), sodium_base64_VARIANT_URLSAFE_NO_PADDING)];
    sodium_bin2base64(encoded, sizeof(encoded), bytes, sizeof(bytes),
                      sodium_base64_VARIANT_URLSAFE_NO_PADDING);

    NewToken result{encoded, {}};
    result.hash = hashToken(result.token);
    return result;
}

std::string hashToken(std::string_view token) {
    unsigned char digest[crypto_hash_sha256_BYTES];
    crypto_hash_sha256(digest, reinterpret_cast<const unsigned char*>(token.data()), token.size());
    char hex[sizeof(digest) * 2 + 1];
    sodium_bin2hex(hex, sizeof(hex), digest, sizeof(digest));
    return hex;
}

}  // namespace vec::refresh
