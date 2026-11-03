#include "auth/Password.h"

#include <sodium.h>

#include <stdexcept>

namespace vec::password {

namespace {

void initSodium() {
    // Thread-safe, and cheap after the first call.
    if (sodium_init() < 0) throw std::runtime_error("libsodium failed to initialise");
}

}  // namespace

std::string hash(const std::string& password) {
    initSodium();
    char stored[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(stored, password.data(), password.size(), crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        throw std::runtime_error("password hashing ran out of memory");
    }
    return stored;
}

bool verify(const std::string& password, const std::string& storedHash) {
    initSodium();
    // crypto_pwhash_str_verify reads a NUL-terminated string, so a hash with an embedded NUL could be
    // read short.
    if (storedHash.find('\0') != std::string::npos) return false;
    return crypto_pwhash_str_verify(storedHash.c_str(), password.data(), password.size()) == 0;
}

}  // namespace vec::password
