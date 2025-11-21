#include "lz77.h"
#include <cstring>
#include <algorithm>

// ============== HELPERS ==============
static inline void writeLiteral(std::vector<uint8_t>& out, uint8_t byte) {
    if (byte < 0x80) {
        out.push_back(byte);
    } else {
        out.push_back(0xFF);
        out.push_back(byte);
    }
}

static inline void writeReference(std::vector<uint8_t>& out, uint16_t length, uint16_t distance) {
    out.push_back(0x80);
    
    if (length < 255) {
        out.push_back((uint8_t)length);
    } else {
        out.push_back(0xFF);
        out.push_back((uint8_t)(length & 0xFF));
        out.push_back((uint8_t)(length >> 8));
    }
    
    out.push_back((uint8_t)(distance & 0xFF));
    out.push_back((uint8_t)(distance >> 8));
}

static inline int calculateLiteralCost(const uint8_t* data, size_t len) {
    int cost = 0;
    for (size_t i = 0; i < len; ++i) {
        cost += (data[i] < 0x80) ? 1 : 2;
    }
    return cost;
}

static inline int calculateReferenceCost(uint16_t length) {
    return (length < 255) ? 4 : 6;
}

// Búsqueda simple en ventana deslizante
static LZ77::Match findBestMatch(const std::vector<uint8_t>& input, 
                                  size_t pos, 
                                  size_t window_size) {
    LZ77::Match best(0, 0);
    
    size_t lookahead_len = std::min(LZ77::LOOKAHEAD_SIZE, input.size() - pos);
    if (lookahead_len < LZ77::MIN_MATCH_LEN) {
        return best;
    }
    
    // Determinar inicio de la ventana
    size_t window_start = (pos > window_size) ? pos - window_size : 0;
    
    // Buscar en la ventana
    for (size_t i = window_start; i < pos; ++i) {
        size_t len = 0;
        size_t max_len = std::min(lookahead_len, input.size() - pos);
        
        // Comparar bytes
        while (len < max_len && input[i + len] == input[pos + len]) {
            len++;
        }
        
        // Si encontramos un match mejor
        if (len >= LZ77::MIN_MATCH_LEN && len > best.length) {
            best.length = len;
            best.position = pos - i;
        }
    }
    
    return best;
}

// ============== COMPRESIÓN ==============
std::vector<uint8_t> LZ77::compress(const std::vector<uint8_t>& input) {
    const size_t N = input.size();
    if (N == 0) return {};
    
    std::vector<uint8_t> out;
    out.reserve(N / 2);
    
    size_t pos = 0;
    
    while (pos < N) {
        // Buscar mejor match
        Match best = findBestMatch(input, pos, WINDOW_SIZE);
        
        // Decidir si usar referencia o literal
        bool use_reference = false;
        
        if (best.length >= MIN_MATCH_LEN) {
            int literal_cost = calculateLiteralCost(&input[pos], best.length);
            int ref_cost = calculateReferenceCost(best.length);
            use_reference = (ref_cost < literal_cost);
        }
        
        if (use_reference) {
            writeReference(out, best.length, best.position);
            pos += best.length;
        } else {
            writeLiteral(out, input[pos]);
            pos++;
        }
    }
    
    return out;
}

// ============== DESCOMPRESIÓN ==============
std::vector<uint8_t> LZ77::decompress(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> out;
    out.reserve(input.size() * 3);
    
    size_t p = 0;
    const size_t N = input.size();
    
    while (p < N) {
        uint8_t first = input[p++];
        
        if (first < 0x80) {
            out.push_back(first);
        } 
        else if (first == 0xFF) {
            if (p >= N) break;
            out.push_back(input[p++]);
        }
        else {
            if (p >= N) break;
            
            uint16_t length = input[p++];
            if (length == 0xFF) {
                if (p + 1 >= N) break;
                length = input[p] | (input[p + 1] << 8);
                p += 2;
            }
            
            if (p + 1 >= N) break;
            uint16_t distance = input[p] | (input[p + 1] << 8);
            p += 2;
            
            if (distance == 0 || distance > out.size() || length == 0) {
                break;
            }
            
            size_t start = out.size() - distance;
            for (size_t i = 0; i < length; ++i) {
                out.push_back(out[start + i]);
            }
        }
    }
    
    return out;
}