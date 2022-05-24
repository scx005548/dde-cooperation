#ifndef SHA256_H
#define SHA256_H

#include <cstdint>
#include <cstddef>

namespace Hash {

/* SHA256 */
typedef struct Sha256State {
    uint32_t A;
    uint32_t B;
    uint32_t C;
    uint32_t D;
    uint32_t E;
    uint32_t F;
    uint32_t G;
    uint32_t H;
} Sha256State;

void sha256Init(Sha256State *state);
void sha256Count(Sha256State *state, const void *data);
void sha256Tail(Sha256State *state, void *data, uint8_t currentBytes, uint64_t totalBytes);
void sha256Result(Sha256State *state, char *result);

void sha256(const void *data, size_t length, char *result);

/* New SHA256 Interface */
typedef struct Sha256 {
    Sha256State state;

    char hex[65];       // sha256 result hex sum
    uint8_t buffer[64]; // buffer
    size_t used;        // length of buffer used
    size_t length;      // length of total data
} Sha256;

void sha256Reset(Sha256 *sha256);
void sha256Update(Sha256 *sha256, const void *data, size_t length);
const char *sha256Hex(Sha256 *sha256);
const char *sha256OfData(Sha256 *sha256, const void *data, size_t length);
const char *sha256OfString(Sha256 *sha256, const char *str);

} // namespace Hash

#endif // !SHA256_H
