#ifndef LZ77_READER_H
#define LZ77_READER_H

#include <vector>
#include <string>
#include <fstream>
#include "common.h"

class LZ77Reader {
public:
    // Leer archivo LZ77 completo
    static std::vector<Token> read_file(const std::string& filename);
    
    // Leer solo el header (para información)
    static void read_header(const std::string& filename, 
                           uint32_t& num_tokens, 
                           uint32_t& original_size);
};

#endif