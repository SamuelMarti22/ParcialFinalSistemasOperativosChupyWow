#include "ChaCha20.h"
#include <cstring>
#include <cstdint>

// === Solo para I/O de archivos y CLI (opcional) ===
#include <fstream>
#include <vector>
#include <stdexcept>
#include <string>
#include <cstdio>
#include <cctype>

// ===== Helpers LE (little-endian) =====
static inline uint32_t load32_le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline void store32_le(uint8_t *out, uint32_t w)
{
    out[0] = (uint8_t)(w);
    out[1] = (uint8_t)(w >> 8);
    out[2] = (uint8_t)(w >> 16);
    out[3] = (uint8_t)(w >> 24);
    
}

static inline void ensure(bool cond, const char* msg) {
    if (!cond) throw std::runtime_error(msg);
}


// ===== Constantes en palabras (LE) de "expand 32-byte k" =====
static const uint32_t CHACHA_CONST[4] = {
    0x61707865u, 0x3320646eu, 0x79622d32u, 0x6b206574u};

// ===== rotl32 y quarter_round (tus funciones, sin cambios) =====
uint32_t rotl32(uint32_t x, uint32_t n)
{
    return ((x << n) & 0xffffffffu) | (x >> (32 - n));
}

void quarter_round(uint32_t *state, int a, int b, int c, int d)
{
    state[a] = (state[a] + state[b]) & 0xffffffffu;
    state[d] ^= state[a];
    state[d] = rotl32(state[d], 16);
    state[c] = (state[c] + state[d]) & 0xffffffffu;
    state[b] ^= state[c];
    state[b] = rotl32(state[b], 12);
    state[a] = (state[a] + state[b]) & 0xffffffffu;
    state[d] ^= state[a];
    state[d] = rotl32(state[d], 8);
    state[c] = (state[c] + state[d]) & 0xffffffffu;
    state[b] ^= state[c];
    state[b] = rotl32(state[b], 7);
}

// ===== init =====
// Guarda key/nonce/counter en el contexto y deja state[] con el "estado base" (opcional).
void chacha20_init(ChaCha20_Context *ctx, const uint8_t *key, const uint8_t *nonce, uint32_t counter)
{
    std::memcpy(ctx->key, key, CHACHA20_KEY_SIZE);
    std::memcpy(ctx->nonce, nonce, CHACHA20_NONCE_SIZE);
    ctx->counter = counter;

    // (Opcional) construir el estado base en ctx->state para inspección/debug:
    uint32_t *st = ctx->state;
    st[0] = CHACHA_CONST[0];
    st[1] = CHACHA_CONST[1];
    st[2] = CHACHA_CONST[2];
    st[3] = CHACHA_CONST[3];
    st[4] = load32_le(&ctx->key[0]);
    st[5] = load32_le(&ctx->key[4]);
    st[6] = load32_le(&ctx->key[8]);
    st[7] = load32_le(&ctx->key[12]);
    st[8] = load32_le(&ctx->key[16]);
    st[9] = load32_le(&ctx->key[20]);
    st[10] = load32_le(&ctx->key[24]);
    st[11] = load32_le(&ctx->key[28]);
    st[12] = ctx->counter;
    st[13] = load32_le(&ctx->nonce[0]);
    st[14] = load32_le(&ctx->nonce[4]);
    st[15] = load32_le(&ctx->nonce[8]);
}

// ===== block =====
void chacha20_block(ChaCha20_Context *ctx, uint8_t *output)
{
    // 1) Construir estado base desde el ctx (clave/nonce/ctr)
    uint32_t st[16];
    st[0] = CHACHA_CONST[0];
    st[1] = CHACHA_CONST[1];
    st[2] = CHACHA_CONST[2];
    st[3] = CHACHA_CONST[3];
    st[4] = load32_le(&ctx->key[0]);
    st[5] = load32_le(&ctx->key[4]);
    st[6] = load32_le(&ctx->key[8]);
    st[7] = load32_le(&ctx->key[12]);
    st[8] = load32_le(&ctx->key[16]);
    st[9] = load32_le(&ctx->key[20]);
    st[10] = load32_le(&ctx->key[24]);
    st[11] = load32_le(&ctx->key[28]);
    st[12] = ctx->counter; // 32-bit block counter
    st[13] = load32_le(&ctx->nonce[0]);
    st[14] = load32_le(&ctx->nonce[4]);
    st[15] = load32_le(&ctx->nonce[8]);

    // 2) Copia de trabajo
    uint32_t x[16];
    std::memcpy(x, st, sizeof(st));

    // 3) 20 rondas (10 dobles: columnas + diagonales)
    for (int i = 0; i < 10; ++i)
    {
        // columnas
        quarter_round(x, 0, 4, 8, 12);
        quarter_round(x, 1, 5, 9, 13);
        quarter_round(x, 2, 6, 10, 14);
        quarter_round(x, 3, 7, 11, 15);
        // diagonales
        quarter_round(x, 0, 5, 10, 15);
        quarter_round(x, 1, 6, 11, 12);
        quarter_round(x, 2, 7, 8, 13);
        quarter_round(x, 3, 4, 9, 14);
    }

    // 4) Suma final + serialización (64 bytes)
    for (int i = 0; i < 16; ++i)
    {
        uint32_t wi = (x[i] + st[i]) & 0xffffffffu;
        store32_le(&output[4 * i], wi);
    }

    // 5) Siguiente bloque
    ctx->counter += 1;
}

void chacha20_xor(ChaCha20_Context *ctx,
                  const uint8_t *in, uint8_t *out, size_t len)
{
    uint8_t block[CHACHA20_BLOCK_SIZE];
    size_t offset = 0;

    while (offset < len)
    {
        // 1) Obtener 64 bytes de keystream (o los que hagan falta)
        chacha20_block(ctx, block); // esto ya incrementa ctx->counter

        // 2) XOR con el trozo correspondiente
        size_t chunk = (len - offset > CHACHA20_BLOCK_SIZE)
                           ? CHACHA20_BLOCK_SIZE
                           : (len - offset);

        for (size_t i = 0; i < chunk; ++i)
        {
            out[offset + i] = in[offset + i] ^ block[i];
        }

        offset += chunk;
    }
}

// === I/O de archivos con ChaCha20 (streaming) ===
void chacha20_xor_file(const std::string& inputPath,
                       const std::string& outputPath,
                       const uint8_t key[CHACHA20_KEY_SIZE],
                       const uint8_t nonce[CHACHA20_NONCE_SIZE],
                       uint32_t counter)
{
    std::ifstream in(inputPath, std::ios::in | std::ios::binary);
    ensure(in.good(), "No se pudo abrir el archivo de entrada");

    std::ofstream out(outputPath, std::ios::out | std::ios::binary | std::ios::trunc);
    ensure(out.good(), "No se pudo crear el archivo de salida");

    ChaCha20_Context ctx{};
    chacha20_init(&ctx, key, nonce, counter);

    const size_t BUF_SIZE = 64 * 1024; // 64 KiB
    std::vector<uint8_t> inBuf(BUF_SIZE);
    std::vector<uint8_t> outBuf(BUF_SIZE);

    while (true) {
        in.read(reinterpret_cast<char*>(inBuf.data()), inBuf.size());
        std::streamsize got = in.gcount();
        if (got <= 0) break;

        chacha20_xor(&ctx, inBuf.data(), outBuf.data(), static_cast<size_t>(got));
        out.write(reinterpret_cast<const char*>(outBuf.data()), got);
        ensure(out.good(), "Error escribiendo en el archivo de salida");
    }

    std::fill(inBuf.begin(), inBuf.end(), 0);
    std::fill(outBuf.begin(), outBuf.end(), 0);
}

// Convierte "A1b2..." -> bytes. Lanza si hay formato inválido.
static std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    auto hexval = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        return -1;
    };

    if (hex.size() % 2 != 0) throw std::runtime_error("Hex con longitud impar");

    std::vector<uint8_t> out(hex.size() / 2);
    for (size_t i = 0; i < out.size(); ++i) {
        int hi = hexval(hex[2*i]);
        int lo = hexval(hex[2*i + 1]);
        if (hi < 0 || lo < 0) throw std::runtime_error("Hex no válido");
        out[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    return out;
}

int main() {
    // 1) KEY y NONCE manuales (ejemplo didáctico; no usar en producción)
    uint8_t key[CHACHA20_KEY_SIZE] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
        0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f
    };
    uint8_t nonce[CHACHA20_NONCE_SIZE] = { 0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x00,0x11,0x22,0x33,0x44,0x55 };
    uint32_t counter = 0;

    // 2) Cifrar archivo
    chacha20_xor_file("lz77.cpp", "salida.wow", key, nonce, counter);

    // 3) Descifrar archivo
    //    (mismas key/nonce/counter)
    chacha20_xor_file("salida.wow", "recuperado.cpp", key, nonce, counter);

    return 0;
}