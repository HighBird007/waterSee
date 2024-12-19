#ifndef AES_H
#define AES_H
#include <stdint.h>
#if 1
#  define AES_ENC_PREKEYED  
#endif
#if 0
#  define AES_DEC_PREKEYED  
#endif
#if 0
#  define AES_ENC_128_OTFK  
#endif
#if 0
#  define AES_DEC_128_OTFK  
#endif
#if 0
#  define AES_ENC_256_OTFK  
#endif
#if 0
#  define AES_DEC_256_OTFK  
#endif
#define N_ROW                   4
#define N_COL                   4
#define N_BLOCK   (N_ROW * N_COL)
#define N_MAX_ROUNDS           14
typedef uint8_t return_type;
typedef uint8_t length_type;
typedef struct
{   uint8_t ksch[(N_MAX_ROUNDS + 1) * N_BLOCK];
    uint8_t rnd;
} aes_context;
#if defined( AES_ENC_PREKEYED ) || defined( AES_DEC_PREKEYED )
return_type aes_set_key( const uint8_t key[],
                         length_type keylen,
                         aes_context ctx[1] );
#endif
#if defined( AES_ENC_PREKEYED )
return_type aes_encrypt( const uint8_t in[N_BLOCK],
                         uint8_t out[N_BLOCK],
                         const aes_context ctx[1] );
return_type aes_cbc_encrypt( const uint8_t *in,
                         uint8_t *out,
                         int32_t n_block,
                         uint8_t iv[N_BLOCK],
                         const aes_context ctx[1] );
#endif
#if defined( AES_DEC_PREKEYED )
return_type aes_decrypt( const uint8_t in[N_BLOCK],
                         uint8_t out[N_BLOCK],
                         const aes_context ctx[1] );
return_type aes_cbc_decrypt( const uint8_t *in,
                         uint8_t *out,
                         int32_t n_block,
                         uint8_t iv[N_BLOCK],
                         const aes_context ctx[1] );
#endif
#if defined( AES_ENC_128_OTFK )
void aes_encrypt_128( const uint8_t in[N_BLOCK],
                      uint8_t out[N_BLOCK],
                      const uint8_t key[N_BLOCK],
                      uint8_t o_key[N_BLOCK] );
#endif
#if defined( AES_DEC_128_OTFK )
void aes_decrypt_128( const uint8_t in[N_BLOCK],
                      uint8_t out[N_BLOCK],
                      const uint8_t key[N_BLOCK],
                      uint8_t o_key[N_BLOCK] );
#endif
#if defined( AES_ENC_256_OTFK )
void aes_encrypt_256( const uint8_t in[N_BLOCK],
                      uint8_t out[N_BLOCK],
                      const uint8_t key[2 * N_BLOCK],
                      uint8_t o_key[2 * N_BLOCK] );
#endif
#if defined( AES_DEC_256_OTFK )
void aes_decrypt_256( const uint8_t in[N_BLOCK],
                      uint8_t out[N_BLOCK],
                      const uint8_t key[2 * N_BLOCK],
                      uint8_t o_key[2 * N_BLOCK] );
#endif
#endif
