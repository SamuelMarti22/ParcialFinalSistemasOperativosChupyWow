// huffman_decompressor.h
#ifndef HUFFMAN_DECOMPRESSOR_H
#define HUFFMAN_DECOMPRESSOR_H

#include <string>
#include <vector>
#include <map>
#include "common.h"
#include "huffman_tree.h"
#include "bit_reader.h"

class HuffmanDecompressor {
private:
    std::shared_ptr<HuffmanNode> root;
    std::map<Token, int, TokenCompare> frequencies;
    
    // Leer las frecuencias desde el archivo
    bool read_frequencies(std::ifstream& file,
                         std::map<Token, int, TokenCompare>& freq);
    
    // Decodificar bits usando el árbol de Huffman
    std::vector<Token> decode_bits(const std::vector<uint8_t>& compressed_data,
                                   const std::shared_ptr<HuffmanNode>& tree_root,
                                   uint32_t num_tokens);  

public:
    // Descomprimir archivo Huffman a archivo LZ77
    bool decompress_file(const std::string& input_huff,
                        const std::string& output_lz77);
    
    // Obtener las frecuencias leídas (para debugging)
    const std::map<Token, int, TokenCompare>& get_frequencies() const {
        return frequencies;
    }
};

#endif