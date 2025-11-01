// huffman_tree.h
#ifndef HUFFMAN_TREE_H
#define HUFFMAN_TREE_H

#include <vector>
#include <map>
#include <memory>
#include "common.h"

// Nodo del árbol de Huffman
struct HuffmanNode {
    Token token;              // Token que representa (solo para hojas)
    int frequency;            // Frecuencia de aparición
    bool is_leaf;             // ¿Es una hoja (tiene token)?
    
    // Punteros a hijos (nullptr si es hoja)
    std::shared_ptr<HuffmanNode> left;
    std::shared_ptr<HuffmanNode> right;
    
    // Constructor para nodo hoja (con token)
    HuffmanNode(const Token& t, int freq) 
        : token(t), frequency(freq), is_leaf(true), 
          left(nullptr), right(nullptr) {}
    
    // Constructor para nodo interno (sin token)
    HuffmanNode(int freq, 
                std::shared_ptr<HuffmanNode> l, 
                std::shared_ptr<HuffmanNode> r)
        : frequency(freq), is_leaf(false), 
          left(l), right(r) {}
};

// Comparador para la cola de prioridad (min-heap)
struct CompareNodes {
    bool operator()(const std::shared_ptr<HuffmanNode>& a,
                   const std::shared_ptr<HuffmanNode>& b) const {
        // Retorna true si 'a' tiene MAYOR prioridad que 'b'
        // (queremos el de MENOR frecuencia primero, por eso >)
        return a->frequency > b->frequency;
    }
};

// Comparador para usar Token como clave en mapas
struct TokenCompare {
    bool operator()(const Token& a, const Token& b) const {
        // Comparar primero por type, luego value, luego distance
        if (a.type != b.type) return a.type < b.type;
        if (a.value != b.value) return a.value < b.value;
        return a.distance < b.distance;
    }
};

// Clase para construir el árbol de Huffman
class HuffmanTree {
private:
    std::shared_ptr<HuffmanNode> root;
    std::map<Token, int, TokenCompare> frequencies;  // Frecuencias de cada token
    
public:
    // Constructor: recibe los tokens del archivo LZ77
    HuffmanTree(const std::vector<Token>& tokens);
    
    // Construir el árbol
    void build_tree();
    
    // Obtener la raíz del árbol
    std::shared_ptr<HuffmanNode> get_root() const { return root; }
    
    // Obtener las frecuencias calculadas
    const std::map<Token, int, TokenCompare>& get_frequencies() const { 
        return frequencies; 
    }
    
    // Imprimir el árbol (para debugging)
    void print_tree() const;
    
private:
    // Función auxiliar para calcular frecuencias
    void calculate_frequencies(const std::vector<Token>& tokens);
    
    // Función auxiliar para imprimir recursivamente
    void print_node(const std::shared_ptr<HuffmanNode>& node, 
                   const std::string& prefix, 
                   bool is_left) const;
};

#endif