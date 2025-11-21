#include "lz77.h"
#include <cstring>
#include <omp.h>

LZ77::Match LZ77::findLongestMatch(const uint8_t* input, size_t input_size) {
    Match best(0,0);
    if (window.empty() || cursor >= input_size) return best;

    const size_t max_look = std::min(LOOKAHEAD_SIZE, input_size - cursor);
    const size_t W = window.size();

    const size_t PARALLEL_THRESHOLD = 4096;  // Solo paralelizar si ventana > 4KB

    if (W >= PARALLEL_THRESHOLD) {
        // Convertir deque a vector para acceso paralelo
        std::vector<uint8_t> win_vec(window.begin(), window.end());

        #pragma omp parallel
        {
            Match local_best(0, 0);

            #pragma omp for nowait
            for (size_t i = 0; i < W; ++i) {
                size_t ml = 0;
                while (ml < max_look && (i + ml) < W && win_vec[i + ml] == input[cursor + ml]) {
                    ++ml;
                }
                if (ml > local_best.length) {
                    local_best.position = static_cast<uint16_t>(W - i);
                    local_best.length = static_cast<uint16_t>(ml);
                }
            }

            #pragma omp critical
            {
                if (local_best.length > best.length) {
                    best = local_best;
                }
            }
        }
    } else {
        // Versión secuencial para ventanas pequeñas
        for (size_t i = 0; i < W; ++i) {
            size_t ml = 0;
            while (ml < max_look && (i + ml) < W && window[i + ml] == input[cursor + ml]) {
                ++ml;
            }
            if (ml > best.length) {
                best.position = static_cast<uint16_t>(W - i);
                best.length = static_cast<uint16_t>(ml);
                if (ml == max_look) break;
            }
        }
    }

    return best;
}

// ==================== FORMATO  ====================
// LITERAL directo (1 byte):
//   - Si byte está en rango 0x00-0x7F (0-127): se escribe directamente
//   - Si byte está en rango 0x80-0xFF (128-255): [0xFF][byte] (2 bytes)
//
// REFERENCIA (5 bytes): [0x80-0xFE][len_low][len_high][dist_low][dist_high]
//   - Marcador 0x80-0xFE indica que es referencia
//   - len y dist en little-endian de 16 bits
// ==================================================================

void LZ77::writeToken(const LZ77Token& t, std::vector<uint8_t>& out) {
    if (t.type == LITERAL) {
        uint8_t byte = static_cast<uint8_t>(t.value & 0xFF);
        
        if (byte < 0x80) {
            // Literal en rango 0-127: escribe directo (1 byte)
            out.push_back(byte);
        } else {
            // Literal >= 128: necesita escape (2 bytes)
            out.push_back(0xFF);  // Marcador de escape
            out.push_back(byte);
        }
    } else {
        // REFERENCIA: usa marcador 0x80 + longitud y distancia
        out.push_back(0x80);  // Marcador de referencia
        
        // Escribir length (16 bits little-endian)
        uint8_t b[2];
        u16le(t.value, b);
        out.insert(out.end(), b, b+2);
        
        // Escribir distance (16 bits little-endian)
        u16le(t.distance, b);
        out.insert(out.end(), b, b+2);
    }
}

LZ77::LZ77Token LZ77::readToken(const uint8_t* p, size_t remaining, size_t& consumed) {
    LZ77Token t;
    consumed = 0;
    if (remaining == 0) return t;

    uint8_t first = p[0];
    
    if (first < 0x80) {
        // Literal directo en rango 0-127
        t.type = LITERAL;
        t.value = first;
        t.distance = 0;
        consumed = 1;
    } 
    else if (first == 0xFF) {
        // Literal con escape (>=128)
        if (remaining < 2) return t; // corrupto
        t.type = LITERAL;
        t.value = p[1];
        t.distance = 0;
        consumed = 2;
    } 
    else {
        // Referencia (0x80-0xFE)
        if (remaining < 5) return t; // corrupto
        t.type = REFERENCE;
        t.value = leu16(p + 1);    // length
        t.distance = leu16(p + 3); // distance
        consumed = 5;
    }
    
    return t;
}

std::vector<uint8_t> LZ77::compress(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> out;
    window.clear();
    cursor = 0;

    const size_t N = input.size();
    if (N == 0) return out;

    auto push_window = [&](uint8_t byte){
        window.push_back(byte);
        if (window.size() > WINDOW_SIZE) window.pop_front();
    };

    while (cursor < N) {
        Match best = findLongestMatch(input.data(), N);
        
        // Solo usar referencia si ahorra bytes
        if (best.length >= MIN_MATCH_LEN) {
            // Calcular costo de usar literales vs referencia
            int literal_total_cost = 0;
            for (size_t i = 0; i < best.length; ++i) {
                uint8_t byte = input[cursor + i];
                literal_total_cost += (byte < 0x80) ? 1 : 2;
            }
            
            int reference_cost = 5;
            
            // Solo usar referencia si es más eficiente
            if (reference_cost < literal_total_cost) {
                writeToken(LZ77Token(REFERENCE, best.length, best.position), out);
                for (size_t k = 0; k < best.length; ++k) {
                    push_window(input[cursor]);
                    ++cursor;
                    if (cursor >= N) break;
                }
            } else {
                // Referencia no vale la pena, usar literal
                writeToken(LZ77Token(LITERAL, input[cursor], 0), out);
                push_window(input[cursor]);
                ++cursor;
            }
        } else {
            // Match muy corto, usar literal
            writeToken(LZ77Token(LITERAL, input[cursor], 0), out);
            push_window(input[cursor]);
            ++cursor;
        }
    }

    return out;
}

std::vector<uint8_t> LZ77::decompress(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> out;
    size_t p = 0;
    const size_t N = input.size();

    while (p < N) {
        size_t consumed = 0;
        LZ77Token t = readToken(&input[p], N - p, consumed);
        if (consumed == 0) break;
        p += consumed;

        if (t.type == LITERAL) {
            out.push_back(static_cast<uint8_t>(t.value & 0xFF));
        } else { // REFERENCE
            uint16_t len = t.value;
            uint16_t dist = t.distance;
            if (dist == 0 || len == 0 || dist > out.size()) {
                break; // corrupto
            }
            size_t start = out.size() - dist;
            for (size_t i = 0; i < len; ++i) {
                out.push_back(out[start + i]);
            }
        }
    }

    return out;
}