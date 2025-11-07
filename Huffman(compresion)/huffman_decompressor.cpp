// huffman_decompressor.cpp
#include "huffman_decompressor.h"
#include <iostream>
#include <fstream>

// huffman_decompressor.cpp

bool HuffmanDecompressor::decompress_file(const std::string& input_huff,
                                          const std::string& output_lz77) {
    std::cout << "=== DESCOMPRESIÓN HUFFMAN ===\n\n";
    
    std::cout << "Paso 1: Abriendo archivo " << input_huff << "...\n";
    
    std::ifstream file(input_huff, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo\n";
        return false;
    }
    
    // ═══════════════════════════════════════════════════════════
    // Leer número de tokens
    // ═══════════════════════════════════════════════════════════
    uint32_t num_tokens_to_decode;
    file.read((char*)&num_tokens_to_decode, 4);
    
    if (file.fail()) {
        std::cerr << "Error: No se pudo leer el número de tokens\n";
        return false;
    }
    
    // ═══════════════════════════════════════════════════════════
    // NUEVO: Leer original_size
    // ═══════════════════════════════════════════════════════════
    uint32_t original_size_from_huff;
    file.read((char*)&original_size_from_huff, 4);
    
    if (file.fail()) {
        std::cerr << "Error: No se pudo leer original_size\n";
        return false;
    }
    
    std::cout << "  Tokens a decodificar: " << num_tokens_to_decode << "\n";
    std::cout << "  Tamaño original: " << original_size_from_huff << " bytes\n";
    
    // ═══════════════════════════════════════════════════════════
    // Leer frecuencias
    // ═══════════════════════════════════════════════════════════
    
    std::cout << "Paso 2: Leyendo frecuencias...\n";
    
    if (!read_frequencies(file, frequencies)) {
        std::cerr << "Error: No se pudieron leer frecuencias\n";
        return false;
    }
    
    std::cout << "  Tokens únicos: " << frequencies.size() << "\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // Leer datos comprimidos
    // ═══════════════════════════════════════════════════════════
    
    std::cout << "Paso 3: Leyendo datos comprimidos...\n";
    
    std::vector<uint8_t> compressed_data;
    
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    size_t header_size = 4 + 4 + 4 + (frequencies.size() * 9);  // ← ACTUALIZADO: +4 por original_size
    size_t compressed_size = file_size - header_size;
    
    file.seekg(header_size);
    compressed_data.resize(compressed_size);
    file.read((char*)compressed_data.data(), compressed_size);
    
    file.close();
    
    std::cout << "  Bytes comprimidos: " << compressed_size << "\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // Reconstruir árbol
    // ═══════════════════════════════════════════════════════════
    
    std::cout << "Paso 4: Reconstruyendo árbol de Huffman...\n";
    
    std::vector<Token> tokens_for_tree;
    for (const auto& par : frequencies) {
        const Token& token = par.first;
        int freq = par.second;
        
        for (int i = 0; i < freq; ++i) {
            tokens_for_tree.push_back(token);
        }
    }
    
    HuffmanTree tree(tokens_for_tree);
    tree.build_tree();
    root = tree.get_root();
    
    if (!root) {
        std::cerr << "Error: No se pudo reconstruir el árbol\n";
        return false;
    }
    
    std::cout << "  Árbol reconstruido\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // Decodificar bits
    // ═══════════════════════════════════════════════════════════
    
    std::cout << "Paso 5: Decodificando bits...\n";
    
    std::vector<Token> decoded_tokens = decode_bits(compressed_data, root, num_tokens_to_decode);
    
    if (decoded_tokens.size() != num_tokens_to_decode) {
        std::cerr << "Error: Se esperaban " << num_tokens_to_decode 
                  << " tokens pero se decodificaron " << decoded_tokens.size() << "\n";
        return false;
    }
    
    std::cout << "  Tokens decodificados: " << decoded_tokens.size() << "\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // Escribir archivo .lz77
    // ═══════════════════════════════════════════════════════════
    
    std::cout << "Paso 6: Escribiendo archivo " << output_lz77 << "...\n";
    
    std::ofstream out(output_lz77, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error: No se pudo crear el archivo\n";
        return false;
    }
    
    // Escribir header LZ77 con el original_size correcto
    uint32_t num_tokens = decoded_tokens.size();
    
    out.write((const char*)&num_tokens, 4);
    out.write((const char*)&original_size_from_huff, 4);  // ← USAR EL VALOR CORRECTO
    
    // Escribir tokens
    for (const Token& token : decoded_tokens) {
        token.write_to(out);
    }
    
    out.close();
    
    std::cout << "\n✓ Descompresión completada exitosamente\n\n";
    
    return true;
}

// ═══════════════════════════════════════════════════════════
// FUNCIÓN PRIVADA: Leer frecuencias del archivo
// ═══════════════════════════════════════════════════════════

bool HuffmanDecompressor::read_frequencies(
    std::ifstream &file,
    std::map<Token, int, TokenCompare> &freq)
{

    // Leer número de tokens únicos
    uint32_t num_unique;
    file.read((char *)&num_unique, 4);

    if (file.fail() || num_unique == 0)
    {
        return false;
    }

    // Leer cada token y su frecuencia
    for (uint32_t i = 0; i < num_unique; ++i)
    {
        // Leer token (5 bytes)
        Token token = Token::read_from(file);

        // Leer frecuencia (4 bytes)
        uint32_t frequency;
        file.read((char *)&frequency, 4);

        if (file.fail())
        {
            return false;
        }

        freq[token] = frequency;
    }

    return true;
}

// ═══════════════════════════════════════════════════════════
// FUNCIÓN PRIVADA: Decodificar bits usando el árbol
// ═══════════════════════════════════════════════════════════

// huffman_decompressor.cpp

std::vector<Token> HuffmanDecompressor::decode_bits(
    const std::vector<uint8_t> &compressed_data,
    const std::shared_ptr<HuffmanNode> &tree_root,
    uint32_t num_tokens)
{ // ← NUEVO PARÁMETRO

    std::vector<Token> tokens;
    tokens.reserve(num_tokens); // ← Reservar espacio

    if (!tree_root || compressed_data.empty())
    {
        return tokens;
    }

    // Caso especial: solo hay un token único
    if (tree_root->is_leaf)
    {
        for (uint32_t i = 0; i < num_tokens; ++i)
        {
            tokens.push_back(tree_root->token);
        }
        return tokens;
    }

    // Crear BitReader
    BitReader reader(compressed_data);

    // Decodificar bit por bit
    std::shared_ptr<HuffmanNode> current = tree_root;

    // ═══════════════════════════════════════════════════════════
    // MODIFICADO: Detenerse al alcanzar num_tokens
    // ═══════════════════════════════════════════════════════════
    while (reader.has_bits() && tokens.size() < num_tokens)
    {
        bool bit = reader.read_bit();

        // Navegar por el árbol
        if (bit)
        {
            current = current->right;
        }
        else
        {
            current = current->left;
        }

        // Verificar si llegamos a nullptr (error)
        if (!current)
        {
            std::cerr << "Error: Navegación inválida en el árbol\n";
            break;
        }

        // Si llegamos a una hoja, encontramos un token
        if (current->is_leaf)
        {
            tokens.push_back(current->token);
            current = tree_root; // Volver a la raíz
        }
    }

    return tokens;
}