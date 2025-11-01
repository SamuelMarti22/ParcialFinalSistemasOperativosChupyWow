// bit_reader.cpp
#include "bit_reader.h"

bool BitReader::read_bit() {
    // Verificar que no hayamos terminado de leer
    if (byte_index >= data.size()) {
        // No hay más bytes para leer
        return false;  // O podrías lanzar una excepción
    }
    
    // Paso 1: Obtener el byte actual
    uint8_t current_byte = data[byte_index];
    
    // Paso 2: Extraer el bit en la posición bit_index
    // Crear máscara: 1 << bit_index
    // Ejemplo: si bit_index=5 → 1 << 5 = 0b00100000
    uint8_t mascara = 1 << bit_index;
    
    // Aplicar AND para extraer solo ese bit
    bool bit = (current_byte & mascara) != 0;
    
    // Paso 3: Avanzar al siguiente bit
    bit_index--;
    
    // Paso 4: Si terminamos el byte, pasar al siguiente
    if (bit_index < 0) {
        byte_index++;
        bit_index = 7;
    }
    
    return bit;
}