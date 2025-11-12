#ifndef CHACHA20_H
#define CHACHA20_H

#include <stdint.h>
#include <cstddef>
#include <string>

// Tamaños de clave y nonce
#define CHACHA20_KEY_SIZE 32
#define CHACHA20_NONCE_SIZE 12
#define CHACHA20_BLOCK_SIZE 64

// Estructura para el estado de ChaCha20
typedef struct
{
    uint32_t state[16];                 // 16 palabras de 32 bits
    uint8_t key[CHACHA20_KEY_SIZE];     // 32 bytes clave
    uint8_t nonce[CHACHA20_NONCE_SIZE]; // 12 bytes nonce
    uint32_t counter;                   // Contador de bloques
} ChaCha20_Context;

// Función para inicializar el contexto con la clave, nonce y contador
void chacha20_init(ChaCha20_Context *ctx, const uint8_t *key, const uint8_t *nonce, uint32_t counter);

// Función para generar un bloque de keystream
void chacha20_block(ChaCha20_Context *ctx, uint8_t *output);

// Función para hacer el xor
void chacha20_xor(ChaCha20_Context *ctx,
                  const uint8_t *in, uint8_t *out, size_t len);

// Función para encriptar o desencriptar un mensaje (XOR con keystream)
void chacha20_xor(ChaCha20_Context *ctx, const uint8_t *input, uint8_t *output, size_t len);
void quarter_round(uint32_t *state, int a, int b, int c, int d);
uint32_t rotl32(uint32_t x, uint32_t n);

void chacha20_xor_file(const std::string& inputPath,
                       const std::string& outputPath,
                       const uint8_t key[CHACHA20_KEY_SIZE],
                       const uint8_t nonce[CHACHA20_NONCE_SIZE],
                       uint32_t counter);


#endif // CHACHA20_H
