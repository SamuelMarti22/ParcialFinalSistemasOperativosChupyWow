// huffman_compressor.h
#ifndef HUFFMAN_COMPRESSOR_H
#define HUFFMAN_COMPRESSOR_H

#include <string>
#include <vector>
#include <map>
#include "common.h"
#include "huffman_tree.h"
#include "bit_writer.h"

class HuffmanCompressor {
private:
    std::map<Token, std::string, TokenCompare> codes;
    std::map<Token, int, TokenCompare> frequencies;
    
    // Escribir las frecuencias al archivo
    void write_frequencies(std::ofstream& file,
                          const std::map<Token, int, TokenCompare>& freq);
    
    // Comprimir los tokens usando los códigos
    std::vector<uint8_t> compress_tokens(
        const std::vector<Token>& tokens,
        const std::map<Token, std::string, TokenCompare>& codes);

public:
    // Comprimir archivo LZ77 a archivo Huffman
    bool compress_file(const std::string& input_lz77,
                      const std::string& output_huff);
    
    // Obtener las frecuencias calculadas (para debugging)
    const std::map<Token, int, TokenCompare>& get_frequencies() const {
        return frequencies;
    }
    
    // Obtener los códigos generados (para debugging)
    const std::map<Token, std::string, TokenCompare>& get_codes() const {
        return codes;
    }
};

#endif