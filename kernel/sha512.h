/* MaXOS -- SHA-512 (FIPS 180-4), used for password hashing.
 * Freestanding implementation, no libc required.
 */
#ifndef MAXOS_SHA512_H
#define MAXOS_SHA512_H

#include <stddef.h>
#include <stdint.h>

#define SHA512_DIGEST_LEN 64
#define SHA512_HEX_LEN    128 /* hex chars, without NUL */

/* Compute SHA-512 of `data` (len bytes) into out[64]. */
void crypt(const void *data, size_t len, uint8_t out[SHA512_DIGEST_LEN]);

/* Same, but writes 128 lowercase hex chars + NUL into out[129]. */
void sha512_hex(const void *data, size_t len, char out[SHA512_HEX_LEN + 1]);

#endif
