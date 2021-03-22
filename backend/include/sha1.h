#ifndef __SAH1_H__
#define __SAH1_H__

#include <stdint.h>

#define HASH_LENGTH 20
#define BLOCK_LENGTH 64
#define HASHSTRING_LENGTH 40 // 2 * HASH_LENGTH

typedef struct sha1nfo {
  uint32_t buffer[BLOCK_LENGTH / 4];
  uint32_t state[HASH_LENGTH / 4];
  uint32_t byteCount;
  uint8_t bufferOffset;
  uint8_t keyBuffer[BLOCK_LENGTH];
  uint8_t innerHash[HASH_LENGTH];
} sha1nfo;

/* public API - prototypes - TODO: doxygen*/

/**
 */
void sha1_init(sha1nfo *s);
/**
 */
void sha1_writebyte(sha1nfo *s, uint8_t data);
/**
 */
void sha1_write(sha1nfo *s, const char *data, size_t len);
/**
 */
uint8_t *sha1_result(sha1nfo *s);
/**
 */
void sha1_initHmac(sha1nfo *s, const uint8_t *key, int keyLength);
/**
 */
uint8_t *sha1_resultHmac(sha1nfo *s);

int sha1_file(const char *filename, char *hash);

/**
 * generates sha1 hash string
 * #268
 */
int sha1_hash(sha1nfo *s, char *hash);

/**
 * generates sha1 hash string from src, returns an error if char *src is empty or NULL
 * #268, #237
 */
int sha1_string(char *src, char *hash);

/**
 * checks a given string against a given hash
 * #268, #237
 */
int sha1_validate_string(char *src, char *hash);

/**
 * weak validation if a string could be a sha1 hash (length, character types)eak validation if a string could be a sha1 hash (length, character types)
 * #268, #237
 */
int sha1_validate_string_hashlike(char *str);

#endif
