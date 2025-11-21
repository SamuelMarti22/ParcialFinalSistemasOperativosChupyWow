#ifndef LZ77_H
#define LZ77_H

#include <cstdint>
#include <vector>

class LZ77 {
public:
    // Configuración óptima (32KB ventana, 258 bytes lookahead)
    static constexpr size_t WINDOW_SIZE     = 32768;  // 32 KB (estándar DEFLATE)
    static constexpr size_t LOOKAHEAD_SIZE  = 258;    // Máximo DEFLATE
    static constexpr size_t MIN_MATCH_LEN   = 3;      // Mínimo útil

    // Estructura interna para matches - AHORA PÚBLICA
    struct Match {
        uint16_t position; // distancia hacia atrás
        uint16_t length;   // longitud del match
        Match(uint16_t p=0, uint16_t l=0): position(p), length(l){}
    };

    // API principal: compresión / descompresión de bytes
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
};

#endif // LZ77_H