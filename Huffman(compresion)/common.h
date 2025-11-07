// common.h
#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <cstddef>
#include <fstream>
#include "lz77.h"  // ← IMPORTAR lz77.h para usar su TokenType

// Ya NO definimos TokenType aquí porque viene de lz77.h
// enum TokenType ya está definido en lz77.h

// Estructura que representa un token de LZ77
// Compatible con LZ77Token de lz77.h
struct Token {
    uint8_t type;       // 0 = LITERAL, 1 = REFERENCE
    uint16_t value;
    uint16_t distance;

    // Constructores
    Token() : type(LITERAL), value(0), distance(0) {}
    
    Token(uint8_t character) 
        : type(LITERAL), value(character), distance(0) {}
    
    Token(uint16_t match_length, uint16_t match_distance)
        : type(REFERENCE), value(match_length), distance(match_distance) {}

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
    
    static constexpr size_t SIZE = 5;
};

#endif