#include "sha256.h"
#include <cstring>

// Constantes K de SHA-256 (primeros 32 bits de las raíces cúbicas de los primeros 64 primos)
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Macros auxiliares para SHA-256
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

SHA256::SHA256() : count_(0) {
    // Valores iniciales de hash (primeros 32 bits de las raíces cuadradas de los primeros 8 primos)
    state_[0] = 0x6a09e667;
    state_[1] = 0xbb67ae85;
    state_[2] = 0x3c6ef372;
    state_[3] = 0xa54ff53a;
    state_[4] = 0x510e527f;
    state_[5] = 0x9b05688c;
    state_[6] = 0x1f83d9ab;
    state_[7] = 0x5be0cd19;
    std::memset(buffer_, 0, 64);
}

void SHA256::transform(const uint8_t* chunk) {
    uint32_t m[64];
    uint32_t a, b, c, d, e, f, g, h, t1, t2;
    
    // Preparar el schedule de mensajes (primeros 16 son los datos directos)
    for (int i = 0; i < 16; ++i) {
        m[i] = (chunk[i * 4] << 24) |
               (chunk[i * 4 + 1] << 16) |
               (chunk[i * 4 + 2] << 8) |
               (chunk[i * 4 + 3]);
    }
    
    // Extender los primeros 16 words a 64 words
    for (int i = 16; i < 64; ++i) {
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    }
    
    // Inicializar variables de trabajo con el estado actual
    a = state_[0];
    b = state_[1];
    c = state_[2];
    d = state_[3];
    e = state_[4];
    f = state_[5];
    g = state_[6];
    h = state_[7];
    
    // 64 rondas principales
    for (int i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    
    // Actualizar el estado con los valores calculados
    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
}

void SHA256::update(const uint8_t* data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        buffer_[count_ % 64] = data[i];
        count_++;
        
        // Cuando completamos un bloque de 512 bits (64 bytes), lo procesamos
        if (count_ % 64 == 0) {
            transform(buffer_);
        }
    }
}

void SHA256::update(const std::string& data) {
    update(reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
}

void SHA256::final(uint8_t digest[32]) {
    // Padding según el estándar SHA-256
    size_t i = count_ % 64;
    buffer_[i++] = 0x80; // Agregar bit '1' seguido de ceros
    
    // Si no hay espacio suficiente para la longitud, procesar este bloque y empezar uno nuevo
    if (i > 56) {
        while (i < 64) {
            buffer_[i++] = 0x00;
        }
        transform(buffer_);
        i = 0;
    }
    
    // Rellenar con ceros hasta dejar espacio para la longitud
    while (i < 56) {
        buffer_[i++] = 0x00;
    }
    
    // Agregar la longitud del mensaje en bits (big-endian, 64 bits)
    uint64_t bit_count = count_ * 8;
    for (int j = 7; j >= 0; --j) {
        buffer_[56 + j] = bit_count & 0xff;
        bit_count >>= 8;
    }
    
    // Procesar el último bloque
    transform(buffer_);
    
    // Producir el hash final en formato big-endian
    for (int i = 0; i < 8; ++i) {
        digest[i * 4] = (state_[i] >> 24) & 0xff;
        digest[i * 4 + 1] = (state_[i] >> 16) & 0xff;
        digest[i * 4 + 2] = (state_[i] >> 8) & 0xff;
        digest[i * 4 + 3] = state_[i] & 0xff;
    }
}

// Función estática de conveniencia para hacer hash en un solo paso
void SHA256::hash(const uint8_t* data, size_t length, uint8_t digest[32]) {
    SHA256 sha;
    sha.update(data, length);
    sha.final(digest);
}

void SHA256::hash(const std::string& data, uint8_t digest[32]) {
    hash(reinterpret_cast<const uint8_t*>(data.c_str()), data.length(), digest);
}
