// common.h
#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <fstream>

// Estructura que representa un token de LZ77
struct Token {
    uint8_t type;       // 0 = LITERAL, 1 = REFERENCE
    uint16_t value;     // Si LITERAL: código ASCII del carácter
                        // Si REFERENCE: longitud de la secuencia repetida
    uint16_t distance;  // Si LITERAL: siempre 0
                        // Si REFERENCE: cuántos bytes atrás está la secuencia

    // Escribir este token a un archivo binario
    void write_to(std::ofstream& file) const {
        file.write((const char*)&type, sizeof(type));
        file.write((const char*)&value, sizeof(value));
        file.write((const char*)&distance, sizeof(distance));
    }
    
    // Leer un token desde un archivo binario
    static Token read_from(std::ifstream& file) {
        Token token;
        file.read((char*)&token.type, sizeof(token.type));
        file.read((char*)&token.value, sizeof(token.value));
        file.read((char*)&token.distance, sizeof(token.distance));
        return token;
    }
    
    // Tamaño en bytes de cada token (para cálculos)
    static constexpr size_t SIZE = sizeof(type) + sizeof(value) + sizeof(distance); // 5 bytes
};

#endif