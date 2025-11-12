#include "lz77.h"
#include <cstring>

LZ77::Match LZ77::findLongestMatch(const uint8_t* input, size_t input_size) {
    Match best(0,0);
    if (window.empty() || cursor >= input_size) return best;

    const size_t max_look = std::min(LOOKAHEAD_SIZE, input_size - cursor);
    const size_t W = window.size();

    for (size_t i = 0; i < W; ++i) {
        size_t ml = 0;
        while (ml < max_look && (i + ml) < W && window[i + ml] == input[cursor + ml]) {
            ++ml;
        }
        if (ml > best.length) {
            best.position = static_cast<uint16_t>(W - i); // distancia hacia atrás
            best.length   = static_cast<uint16_t>(ml);
            if (ml == max_look) break; // no se puede mejorar
        }
    }
    return best;
}

// literal = 2 bytes: [0x00][literal]
// referencia = 5 bytes: [0x01][len LE16][dist LE16]
void LZ77::writeToken(const LZ77Token& t, std::vector<uint8_t>& out) {
    if (t.type == LITERAL) {
        out.push_back(LITERAL);
        out.push_back(static_cast<uint8_t>(t.value & 0xFF));
    } else {
        out.push_back(REFERENCE);
        uint8_t b[2];
        u16le(t.value, b);     // len
        out.insert(out.end(), b, b+2);
        u16le(t.distance, b);  // dist
        out.insert(out.end(), b, b+2);
    }
}

LZ77::LZ77Token LZ77::readToken(const uint8_t* p, size_t remaining, size_t& consumed) {
    LZ77Token t;
    consumed = 0;
    if (remaining == 0) return t;

    t.type = p[0];

    if (t.type == LITERAL) {
        if (remaining < 2) return t; // corrupto
        t.value = p[1];
        t.distance = 0;
        consumed = 2;
    } else {
        if (remaining < 5) return t; // corrupto
        t.value    = leu16(p + 1); // len
        t.distance = leu16(p + 3); // dist
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
        // llenar ventana con lo anterior a cursor
        // (en esta implementación la vamos manteniendo incrementalmente)
        // No hay que hacer nada especial aquí.

        // buscar mejor match
        Match best = findLongestMatch(input.data(), N);

        // decidimos referencia sólo si length >= MIN_MATCH_LEN
        if (best.length >= MIN_MATCH_LEN) {
            // emitir referencia
            writeToken(LZ77Token(REFERENCE, best.length, best.position), out);
            // y avanzar 'best.length' bytes llevando a ventana
            for (size_t k = 0; k < best.length; ++k) {
                push_window(input[cursor]);
                ++cursor;
                if (cursor >= N) break;
            }
        } else {
            // emitir literal
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
        if (consumed == 0) break; // corrupto o fin inesperado
        p += consumed;

        if (t.type == LITERAL) {
            out.push_back(static_cast<uint8_t>(t.value & 0xFF));
        } else { // REFERENCE
            uint16_t len = t.value;
            uint16_t dist = t.distance;
            if (dist == 0 || len == 0 || dist > out.size()) {
                // corrupto
                break;
            }
            // copiar con posible solapamiento (desde out)
            size_t start = out.size() - dist;
            for (size_t i = 0; i < len; ++i) {
                out.push_back(out[start + i]);
            }
        }
    }

    return out;
}
