#ifndef LZ77_H
#define LZ77_H

#include <cstdint>
#include <vector>
#include <deque>
#include <algorithm>

class LZ77 {
public:
    enum : uint8_t { LITERAL = 0x00, REFERENCE = 0x01 };

    struct LZ77Token {
        uint8_t  type;      // 0 = literal, 1 = referencia
        uint16_t value;     // literal: byte; referencia: length
        uint16_t distance;  // referencia: distancia hacia atrás
        LZ77Token(uint8_t t=0, uint16_t v=0, uint16_t d=0) : type(t), value(v), distance(d) {}
    };

    // Ajusta estos tamaños a tu gusto
    static constexpr size_t WINDOW_SIZE     = 32 * 1024;  // 32 KiB ventana
    static constexpr size_t LOOKAHEAD_SIZE  = 258;        // típico DEFLATE
    static constexpr size_t MIN_MATCH_LEN   = 3;

    // API: compresión / descompresión de bytes
    std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);

private:
    // estado por bloque (se resetea por llamada)
    std::deque<uint8_t> window;
    size_t cursor = 0; // índice en el input durante la compresión

    struct Match {
        uint16_t position; // distancia
        uint16_t length;   // largo
        Match(uint16_t p=0, uint16_t l=0): position(p), length(l){}
    };

    Match findLongestMatch(const uint8_t* input, size_t input_size);

    // Serialización variable: literal=2B, ref=5B
    static void writeToken(const LZ77Token& t, std::vector<uint8_t>& out);
    static LZ77Token readToken(const uint8_t* p, size_t remaining, size_t& consumed);

    // helpers LE16
    static inline void u16le(uint16_t v, uint8_t* out) {
        out[0] = static_cast<uint8_t>(v & 0xFF);
        out[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
    }
    static inline uint16_t leu16(const uint8_t* p) {
        return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
    }
};

#endif
