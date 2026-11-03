#ifndef SERVER_PASSWORD_H
#define SERVER_PASSWORD_H

#include <string>

namespace vec::password {

// Hashes `password` with argon2id and a random salt. The result holds the salt and settings too, so
// it's all verify() needs. Slow and memory-heavy on purpose (about 64 MB), so keep it off Drogon's
// event-loop threads. Throws std::runtime_error if libsodium can't start or runs out of memory.
std::string hash(const std::string& password);

// Whether `password` matches `storedHash`. False for anything that isn't a valid hash. Takes as long as
// hash() does.
bool verify(const std::string& password, const std::string& storedHash);

}  // namespace vec::password

#endif  // SERVER_PASSWORD_H
