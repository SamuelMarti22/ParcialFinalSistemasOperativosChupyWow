// bit_writer.cpp
#include "bit_writer.h"

void BitWriter::write_bit(bool bit) {
    // Paso 1: Si el bit es 1, encenderlo en la posición correcta
    if (bit) {
        // Calcular en qué posición escribir (7, 6, 5, ..., 0)
        int posicion = 7 - bits_written;
        
        // Crear una máscara con un 1 en esa posición
        // Ejemplo: si posicion=5 → 1 << 5 = 0b00100000
        uint8_t mascara = 1 << posicion;
        
        // Encender el bit usando OR
        current_byte = current_byte | mascara;
    }
    // Si bit es 0, no hacemos nada (ya está en 0)
    
    // Paso 2: Incrementar el contador
    bits_written++;
    
    // Paso 3: Verificar si completamos un byte
    if (bits_written == 8) {
        // Guardar el byte completo en buffer
        buffer.push_back(current_byte);
        
        // Reiniciar para el siguiente byte
        current_byte = 0;
        bits_written = 0;
    }
}

std::vector<uint8_t> BitWriter::get_bytes() {
    // Si hay bits pendientes, hacer flush
    if (bits_written > 0) {
        buffer.push_back(current_byte);
        current_byte = 0;
        bits_written = 0;
    }
    
    return buffer;
}