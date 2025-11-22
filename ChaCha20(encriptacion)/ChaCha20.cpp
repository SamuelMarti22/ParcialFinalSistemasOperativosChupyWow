#include "ChaCha20.h"
#include "sha256.h"
#include <cstring>
#include <cstdint>
#include <omp.h>

// === Solo para I/O de archivos y CLI (opcional) ===
#include <fstream>
#include <vector>
#include <stdexcept>
#include <string>
#include <cstdio>
#include <cctype>
#include <random>
#include <iostream>
#include <termios.h>
#include <unistd.h>

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
        // columnas - paralelizadas
        #pragma omp parallel sections
        {
            #pragma omp section
            quarter_round(x, 0, 4, 8, 12);
            #pragma omp section
            quarter_round(x, 1, 5, 9, 13);
            #pragma omp section
            quarter_round(x, 2, 6, 10, 14);
            #pragma omp section
            quarter_round(x, 3, 7, 11, 15);
        }
        
        // diagonales - paralelizadas
        #pragma omp parallel sections
        {
            #pragma omp section
            quarter_round(x, 0, 5, 10, 15);
            #pragma omp section
            quarter_round(x, 1, 6, 11, 12);
            #pragma omp section
            quarter_round(x, 2, 7, 8, 13);
            #pragma omp section
            quarter_round(x, 3, 4, 9, 14);
        }
    }

    // 4) Suma final + serialización (64 bytes) - paralelizado
    #pragma omp parallel for
    for (int i = 0; i < 16; ++i)
    {
        uint32_t wi = (x[i] + st[i]) & 0xffffffffu;
        store32_le(&output[4 * i], wi);
    }

    // 5) Siguiente bloque
    ctx->counter += 1;
}

// Función auxiliar que genera un bloque con un contador específico sin modificar el contexto
static void chacha20_block_with_counter(const uint8_t key[CHACHA20_KEY_SIZE],
                                         const uint8_t nonce[CHACHA20_NONCE_SIZE],
                                         uint32_t counter,
                                         uint8_t *output)
{
    // 1) Construir estado base
    uint32_t st[16];
    st[0] = CHACHA_CONST[0];
    st[1] = CHACHA_CONST[1];
    st[2] = CHACHA_CONST[2];
    st[3] = CHACHA_CONST[3];
    st[4] = load32_le(&key[0]);
    st[5] = load32_le(&key[4]);
    st[6] = load32_le(&key[8]);
    st[7] = load32_le(&key[12]);
    st[8] = load32_le(&key[16]);
    st[9] = load32_le(&key[20]);
    st[10] = load32_le(&key[24]);
    st[11] = load32_le(&key[28]);
    st[12] = counter;
    st[13] = load32_le(&nonce[0]);
    st[14] = load32_le(&nonce[4]);
    st[15] = load32_le(&nonce[8]);

    // 2) Copia de trabajo
    uint32_t x[16];
    std::memcpy(x, st, sizeof(st));

    // 3) 20 rondas
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

    // 4) Suma final + serialización
    for (int i = 0; i < 16; ++i)
    {
        uint32_t wi = (x[i] + st[i]) & 0xffffffffu;
        store32_le(&output[4 * i], wi);
    }
}

void chacha20_xor(ChaCha20_Context *ctx,
                  const uint8_t *in, uint8_t *out, size_t len)
{
    // Calcular número de bloques completos
    size_t num_blocks = len / CHACHA20_BLOCK_SIZE;
    size_t remaining = len % CHACHA20_BLOCK_SIZE;
    
    // Procesar bloques completos en paralelo
    if (num_blocks > 0)
    {
        #pragma omp parallel for schedule(dynamic, 4) if(num_blocks >= 4)
        for (size_t block_idx = 0; block_idx < num_blocks; ++block_idx)
        {
            uint8_t block[CHACHA20_BLOCK_SIZE];
            uint32_t block_counter = ctx->counter + static_cast<uint32_t>(block_idx);
            size_t offset = block_idx * CHACHA20_BLOCK_SIZE;
            
            // Generar keystream para este bloque específico
            chacha20_block_with_counter(ctx->key, ctx->nonce, block_counter, block);
            
            // XOR con los datos de entrada
            for (size_t i = 0; i < CHACHA20_BLOCK_SIZE; ++i)
            {
                out[offset + i] = in[offset + i] ^ block[i];
            }
        }
        
        // Actualizar contador del contexto
        ctx->counter += static_cast<uint32_t>(num_blocks);
    }
    
    // Procesar bytes restantes (menos de un bloque completo)
    if (remaining > 0)
    {
        uint8_t block[CHACHA20_BLOCK_SIZE];
        size_t offset = num_blocks * CHACHA20_BLOCK_SIZE;
        
        chacha20_block(ctx, block);
        
        for (size_t i = 0; i < remaining; ++i)
        {
            out[offset + i] = in[offset + i] ^ block[i];
        }
    }
}

// === I/O de archivos con ChaCha20 (streaming) ===

// Función auxiliar para generar nonce aleatorio
static void generate_random_nonce(uint8_t nonce[CHACHA20_NONCE_SIZE]) {
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (urandom.good()) {
        urandom.read(reinterpret_cast<char*>(nonce), CHACHA20_NONCE_SIZE);
    } else {
        // Fallback: usar std::random si /dev/urandom no está disponible
        std::random_device rd;
        for (int i = 0; i < CHACHA20_NONCE_SIZE; i++) {
            nonce[i] = static_cast<uint8_t>(rd() & 0xFF);
        }
    }
}

// Cifrar archivo: genera nonce aleatorio y lo guarda al inicio del archivo cifrado
void chacha20_encrypt_file(const std::string& inputPath,
                           const std::string& outputPath,
                           const uint8_t key[CHACHA20_KEY_SIZE])
{
    std::ifstream in(inputPath, std::ios::in | std::ios::binary);
    ensure(in.good(), "No se pudo abrir el archivo de entrada");

    std::ofstream out(outputPath, std::ios::out | std::ios::binary | std::ios::trunc);
    ensure(out.good(), "No se pudo crear el archivo de salida");

    // Generar nonce aleatorio
    uint8_t nonce[CHACHA20_NONCE_SIZE];
    generate_random_nonce(nonce);

    // Escribir el nonce al inicio del archivo cifrado
    out.write(reinterpret_cast<const char*>(nonce), CHACHA20_NONCE_SIZE);
    ensure(out.good(), "Error escribiendo el nonce");

    // Inicializar ChaCha20 con counter = 0
    ChaCha20_Context ctx{};
    chacha20_init(&ctx, key, nonce, 0);

    const size_t BUF_SIZE = 64 * 1024; // 64 KiB
    std::vector<uint8_t> inBuf(BUF_SIZE);
    std::vector<uint8_t> outBuf(BUF_SIZE);

    // Cifrar el archivo
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

// Descifrar archivo: lee el nonce del inicio del archivo cifrado
void chacha20_decrypt_file(const std::string& inputPath,
                           const std::string& outputPath,
                           const uint8_t key[CHACHA20_KEY_SIZE])
{
    std::ifstream in(inputPath, std::ios::in | std::ios::binary);
    ensure(in.good(), "No se pudo abrir el archivo de entrada");

    // Leer el nonce desde el inicio del archivo
    uint8_t nonce[CHACHA20_NONCE_SIZE];
    in.read(reinterpret_cast<char*>(nonce), CHACHA20_NONCE_SIZE);
    ensure(in.gcount() == CHACHA20_NONCE_SIZE, "Archivo demasiado corto o corrupto");

    std::ofstream out(outputPath, std::ios::out | std::ios::binary | std::ios::trunc);
    ensure(out.good(), "No se pudo crear el archivo de salida");

    // Inicializar ChaCha20 con counter = 0
    ChaCha20_Context ctx{};
    chacha20_init(&ctx, key, nonce, 0);

    const size_t BUF_SIZE = 64 * 1024; // 64 KiB
    std::vector<uint8_t> inBuf(BUF_SIZE);
    std::vector<uint8_t> outBuf(BUF_SIZE);

    // Descifrar el archivo
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

// Función legacy para compatibilidad (si alguien quiere pasar nonce y counter manualmente)
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

// ===== Funciones para derivar clave y leer password =====

// Función para derivar clave desde password usando SHA-256
void derive_key_from_password(const std::string& password, uint8_t key[CHACHA20_KEY_SIZE]) {
    SHA256::hash(password, key);
}

// Función para leer password de forma segura (sin mostrar en pantalla)
std::string read_password(const std::string& prompt) {
    std::cout << prompt;
    std::cout.flush();
    
    // Desactivar echo del terminal
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    std::string password;
    std::getline(std::cin, password);
    
    // Reactivar echo
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    
    std::cout << std::endl;
    return password;
}

/*
int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Uso: " << argv[0] << " <encrypt|decrypt> <archivo_entrada> <archivo_salida>\n";
        std::cerr << "\nEjemplos:\n";
        std::cerr << "  Cifrar:    " << argv[0] << " encrypt documento.txt documento.wow\n";
        std::cerr << "  Descifrar: " << argv[0] << " decrypt documento.wow documento.txt\n";
        std::cerr << "\nTambién puedes usar 'e' o 'd' como abreviación:\n";
        std::cerr << "  " << argv[0] << " e documento.txt documento.wow\n";
        std::cerr << "  " << argv[0] << " d documento.wow documento.txt\n";
        return 1;
    }

    std::string mode = argv[1];
    std::string inputPath = argv[2];
    std::string outputPath = argv[3];

    // Leer password del usuario (no se muestra en pantalla)
    std::string password = read_password("Ingrese la contraseña: ");
    
    if (password.empty()) {
        std::cerr << "Error: La contraseña no puede estar vacía\n";
        return 1;
    }

    // Derivar clave de 32 bytes desde el password usando SHA-256
    uint8_t key[CHACHA20_KEY_SIZE];
    derive_key_from_password(password, key);

    // Limpiar password de la memoria por seguridad
    std::fill(password.begin(), password.end(), '\0');

    try {
        if (mode == "encrypt" || mode == "e") {
            chacha20_encrypt_file(inputPath, outputPath, key);
            std::cout << "✓ Archivo cifrado exitosamente: " << outputPath << "\n";
        } 
        else if (mode == "decrypt" || mode == "d") {
            chacha20_decrypt_file(inputPath, outputPath, key);
            std::cout << "✓ Archivo descifrado exitosamente: " << outputPath << "\n";
        }
        else {
            std::cerr << "Error: Modo inválido. Use 'encrypt' o 'decrypt' (o 'e'/'d')\n";
            // Limpiar clave antes de salir
            std::fill(key, key + CHACHA20_KEY_SIZE, 0);
            return 1;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        // Limpiar clave antes de salir
        std::fill(key, key + CHACHA20_KEY_SIZE, 0);
        return 1;
    }

    // Limpiar clave de la memoria por seguridad
    std::fill(key, key + CHACHA20_KEY_SIZE, 0);

    return 0;
}
*/