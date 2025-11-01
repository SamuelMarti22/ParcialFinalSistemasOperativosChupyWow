#include "lz77_reader.h"
#include <iostream>

std::vector<Token> LZ77Reader::read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir " << filename << std::endl;
        return {};
    }
    
    // Leer header
    uint32_t num_tokens, original_size;
    file.read((char*)&num_tokens, 4);
    file.read((char*)&original_size, 4);
    
    std::cout << "Leyendo " << num_tokens << " tokens (tamaño original: " 
              << original_size << " bytes)\n";
    
    // Leer tokens
    std::vector<Token> tokens;
    tokens.reserve(num_tokens);  // Optimización: reservar espacio
    
    for (uint32_t i = 0; i < num_tokens; i++) {
        Token token = Token::read_from(file);
        tokens.push_back(token);
    }
    
    file.close();
    return tokens;
}

void LZ77Reader::read_header(const std::string& filename,
                              uint32_t& num_tokens,
                              uint32_t& original_size) {
    std::ifstream file(filename, std::ios::binary);
    
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir " << filename << std::endl;
        num_tokens = 0;
        original_size = 0;
        return;
    }
    
    file.read((char*)&num_tokens, 4);
    file.read((char*)&original_size, 4);
    
    file.close();
}