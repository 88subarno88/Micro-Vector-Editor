#include <openssl/crypto.h>

// Database clients are never destroyed (see DatabaseTest.h), so their threads are still running when a test
// process exits, sometimes midway through logging in to Postgres, which uses OpenSSL. OpenSSL's own clean-up
// at exit would free memory those threads are still reading. Skip it: the OS reclaims everything anyway.
namespace {

[[maybe_unused]] const bool openSslInitialized = OPENSSL_init_crypto(OPENSSL_INIT_NO_ATEXIT, nullptr) == 1;

}  // namespace
