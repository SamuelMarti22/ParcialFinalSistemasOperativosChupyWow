// bit_writer.h
#ifndef BIT_WRITER_H
#define BIT_WRITER_H

#include <vector>
#include <cstdint>

class BitWriter {
private:
    std::vector<uint8_t> buffer;    // ← Necesitas declarar esto
    uint8_t current_byte;           // ← Necesitas declarar esto
    int bits_written;               // ← Necesitas declarar esto (¡aquí está el problema!)
    
public:
    // Constructor
    BitWriter() : current_byte(0), bits_written(0) {}
    
    // Escribir un solo bit
    void write_bit(bool bit);
    
    // Obtener todos los bytes (hace flush del byte actual)
    std::vector<uint8_t> get_bytes();
    
    // Cuántos bits hemos escrito en total
    int total_bits() const {
        return buffer.size() * 8 + bits_written;
    }
};

#endif