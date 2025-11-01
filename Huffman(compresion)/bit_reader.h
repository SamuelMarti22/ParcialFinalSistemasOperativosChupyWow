// bit_reader.h
#ifndef BIT_READER_H
#define BIT_READER_H

#include <vector>
#include <cstdint>

class BitReader {
private:
    const std::vector<uint8_t>& data;  // Referencia a los bytes a leer
    size_t byte_index;                  // ¿En qué byte estamos?
    int bit_index;                      // ¿En qué bit del byte actual? (7→0)
    
public:
    // Constructor: recibe los bytes a leer
    BitReader(const std::vector<uint8_t>& bytes) 
        : data(bytes), byte_index(0), bit_index(7) {}
    
    // Leer un solo bit
    bool read_bit();
    
    // ¿Hay más bits para leer?
    bool has_bits() const {
        return byte_index < data.size();
    }
};

#endif