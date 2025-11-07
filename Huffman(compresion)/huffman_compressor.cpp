// huffman_compressor.cpp
#include "huffman_compressor.h"
#include "lz77_reader.h"
#include <iostream>
#include <fstream>

// huffman_compressor.cpp

bool HuffmanCompressor::compress_file(const std::string& input_lz77,
                                      const std::string& output_huff) {
    std::cout << "=== COMPRESIÓN HUFFMAN ===\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // PASO 1: Leer tokens Y original_size del archivo LZ77
    // ═══════════════════════════════════════════════════════════
    
    std::cout << "Paso 1: Leyendo archivo LZ77...\n";
    
    std::ifstream lz77_file(input_lz77, std::ios::binary);
    if (!lz77_file.is_open()) {
        std::cerr << "Error: No se pudo abrir " << input_lz77 << "\n";
        return false;
    }
    
    // Leer header LZ77
    uint32_t num_tokens_lz77, original_size_lz77;
    lz77_file.read((char*)&num_tokens_lz77, 4);
    lz77_file.read((char*)&original_size_lz77, 4);
    
    // Leer tokens
    std::vector<Token> tokens;
    tokens.reserve(num_tokens_lz77);
    
    for (uint32_t i = 0; i < num_tokens_lz77; i++) {
        Token token = Token::read_from(lz77_file);
        tokens.push_back(token);
    }
    
    lz77_file.close();
    
    if (tokens.empty()) {
        std::cerr << "Error: No se pudieron leer tokens\n";
        return false;
    }
    
    std::cout << "  Tokens leídos: " << tokens.size() << "\n";
    std::cout << "  Tamaño original: " << original_size_lz77 << " bytes\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // PASO 2: Construir árbol Huffman y generar códigos
    // ═══════════════════════════════════════════════════════════
    
    std::cout << "Paso 2: Construyendo árbol Huffman...\n";
    HuffmanTree tree(tokens);
    tree.build_tree();
    
    std::cout << "Paso 3: Generando códigos Huffman...\n";
    codes = tree.generate_codes();
    
    if (codes.empty()) {
        std::cerr << "Error: No se pudieron generar códigos\n";
        return false;
    }
    
    frequencies = tree.get_frequencies();
    
    // ═══════════════════════════════════════════════════════════
    // PASO 3: Comprimir tokens a bits
    // ═══════════════════════════════════════════════════════════
    
    std::cout << "Paso 4: Comprimiendo tokens...\n";
    std::vector<uint8_t> compressed_bits = compress_tokens(tokens, codes);
    
    std::cout << "  Bits comprimidos: " << compressed_bits.size() << " bytes\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // PASO 4: Escribir archivo .huff
    // ═══════════════════════════════════════════════════════════
    
    std::cout << "Paso 5: Escribiendo archivo " << output_huff << "...\n";
    
    std::ofstream file(output_huff, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo crear el archivo\n";
        return false;
    }
    
    // Escribir número de tokens
    uint32_t num_tokens = tokens.size();
    file.write((const char*)&num_tokens, 4);
    
    // NUEVO: Escribir original_size
    file.write((const char*)&original_size_lz77, 4);
    
    // Escribir frecuencias
    write_frequencies(file, frequencies);
    
    // Escribir datos comprimidos
    file.write((const char*)compressed_bits.data(), compressed_bits.size());
    
    file.close();
    
    // ═══════════════════════════════════════════════════════════
    // ESTADÍSTICAS
    // ═══════════════════════════════════════════════════════════
    
    std::cout << "\n✓ Compresión completada exitosamente\n\n";
    
    std::cout << "=== ESTADÍSTICAS ===\n";
    std::cout << "Tokens originales: " << tokens.size() << "\n";
    
    int total_bits = 0;
    for (const Token& token : tokens) {
        total_bits += codes[token].length();
    }
    
    std::cout << "Bits sin comprimir: " << tokens.size() * 8 << " bits\n";
    std::cout << "Bits con Huffman: " << total_bits << " bits\n";
    std::cout << "Bytes finales (con frecuencias): " 
              << (8 + frequencies.size() * 9 + compressed_bits.size()) << " bytes\n";
    std::cout << "Ratio de compresión: " 
              << (100.0 * total_bits) / (tokens.size() * 8) << "%\n\n";
    
    return true;
}

// ═══════════════════════════════════════════════════════════
// FUNCIÓN PRIVADA: Escribir frecuencias al archivo
// ═══════════════════════════════════════════════════════════

void HuffmanCompressor::write_frequencies(
    std::ofstream &file,
    const std::map<Token, int, TokenCompare> &freq)
{

    // Escribir número de tokens únicos (4 bytes)
    uint32_t num_unique = freq.size();
    file.write((const char *)&num_unique, 4);

    // Escribir cada token y su frecuencia
    for (const auto &par : freq)
    {
        const Token &token = par.first;
        int frequency = par.second;

        // Token (5 bytes)
        token.write_to(file);

        // Frecuencia (4 bytes)
        uint32_t freq_u32 = frequency;
        file.write((const char *)&freq_u32, 4);
    }
}

// ═══════════════════════════════════════════════════════════
// FUNCIÓN PRIVADA: Comprimir tokens usando códigos
// ═══════════════════════════════════════════════════════════

std::vector<uint8_t> HuffmanCompressor::compress_tokens(
    const std::vector<Token> &tokens,
    const std::map<Token, std::string, TokenCompare> &codes)
{

    BitWriter writer;

    // Para cada token, escribir su código Huffman
    for (const Token &token : tokens)
    {
        // Buscar el código del token
        auto it = codes.find(token);

        if (it == codes.end())
        {
            std::cerr << "Error: Token sin código Huffman\n";
            continue;
        }

        const std::string &code = it->second;

        // Escribir cada bit del código
        for (char bit_char : code)
        {
            bool bit = (bit_char == '1');
            writer.write_bit(bit);
        }
    }

    // Obtener los bytes finales (con flush automático)
    return writer.get_bytes();
}