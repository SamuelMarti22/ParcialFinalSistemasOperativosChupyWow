// huffman_tree.cpp
#include "huffman_tree.h"
#include <queue>
#include <iostream>
#include <iomanip>

// Constructor: recibe los tokens y calcula frecuencias
HuffmanTree::HuffmanTree(const std::vector<Token> &tokens)
{
    calculate_frequencies(tokens);
}

// Función para calcular las frecuencias de cada token
void HuffmanTree::calculate_frequencies(const std::vector<Token> &tokens)
{
    std::cout << "=== Calculando frecuencias ===\n";

    // Contar cuántas veces aparece cada token
    for (const Token &token : tokens)
    {
        frequencies[token]++;
    }

    // Mostrar frecuencias
    std::cout << "Total de tokens únicos: " << frequencies.size() << "\n\n";

    int i = 0;
    for (const auto &par : frequencies)
    {
        const Token &token = par.first;
        int freq = par.second;

        std::cout << "Token " << i << ": ";

        if (token.type == 0)
        {
            // LITERAL
            char c = (char)token.value;
            if (c >= 32 && c <= 126)
            {
                std::cout << "LITERAL '" << c << "'";
            }
            else
            {
                std::cout << "LITERAL (ASCII " << token.value << ")";
            }
        }
        else
        {
            // REFERENCE
            std::cout << "REFERENCE (len=" << token.value
                      << ", dist=" << token.distance << ")";
        }

        std::cout << " → aparece " << freq << " vez(es)\n";
        i++;
    }

    std::cout << "\n";
}

// Construir el árbol de Huffman usando el algoritmo greedy
void HuffmanTree::build_tree()
{
    std::cout << "=== Construyendo árbol de Huffman ===\n\n";

    // Verificar que haya tokens
    if (frequencies.empty())
    {
        std::cerr << "Error: No hay tokens para construir el árbol\n";
        return;
    }

    // Caso especial: solo hay 1 token único
    if (frequencies.size() == 1)
    {
        std::cout << "Solo hay 1 token único, creando árbol simple\n";
        auto par = *frequencies.begin();
        root = std::make_shared<HuffmanNode>(par.first, par.second);
        return;
    }

    // Paso 1: Crear una cola de prioridad (min-heap)
    std::priority_queue<
        std::shared_ptr<HuffmanNode>,
        std::vector<std::shared_ptr<HuffmanNode>>,
        CompareNodes>
        pq;

    // Paso 2: Crear nodos hoja para cada token y agregarlos a la cola
    std::cout << "Paso 1: Creando nodos hoja\n";
    for (const auto &par : frequencies)
    {
        auto nodo = std::make_shared<HuffmanNode>(par.first, par.second);
        pq.push(nodo);

        if (par.first.type == 0)
        {
            char c = (char)par.first.value;
            if (c >= 32 && c <= 126)
            {
                std::cout << "  Nodo hoja: '" << c << "' (freq=" << par.second << ")\n";
            }
            else
            {
                std::cout << "  Nodo hoja: ASCII " << par.first.value
                          << " (freq=" << par.second << ")\n";
            }
        }
        else
        {
            std::cout << "  Nodo hoja: REF(" << par.first.value
                      << "," << par.first.distance
                      << ") (freq=" << par.second << ")\n";
        }
    }

    std::cout << "\nPaso 2: Combinando nodos\n";

    // Paso 3: Algoritmo de construcción
    int paso = 1;
    while (pq.size() > 1)
    {
        // Sacar los dos nodos con menor frecuencia
        auto left = pq.top();
        pq.pop();

        auto right = pq.top();
        pq.pop();

        // Crear un nodo padre con la suma de frecuencias
        int combined_freq = left->frequency + right->frequency;
        auto parent = std::make_shared<HuffmanNode>(combined_freq, left, right);

        std::cout << "  Paso " << paso << ": Combinar nodos con freq "
                  << left->frequency << " + " << right->frequency
                  << " = " << combined_freq << "\n";

        // Agregar el nodo padre de vuelta a la cola
        pq.push(parent);
        paso++;
    }

    // El último nodo en la cola es la raíz del árbol
    root = pq.top();

    std::cout << "\n✓ Árbol construido exitosamente\n";
    std::cout << "  Raíz tiene frecuencia total: " << root->frequency << "\n\n";
}

// Función auxiliar para imprimir el árbol recursivamente
void HuffmanTree::print_node(const std::shared_ptr<HuffmanNode> &node,
                             const std::string &prefix,
                             bool is_left) const
{
    if (!node)
        return;

    std::cout << prefix;
    std::cout << (is_left ? "├─L─ " : "└─R─ ");

    // Mostrar información del nodo
    if (node->is_leaf)
    {
        // Es una hoja, mostrar el token
        if (node->token.type == 0)
        {
            char c = (char)node->token.value;
            if (c >= 32 && c <= 126)
            {
                std::cout << "'" << c << "'";
            }
            else
            {
                std::cout << "ASCII " << node->token.value;
            }
        }
        else
        {
            std::cout << "REF(" << node->token.value
                      << "," << node->token.distance << ")";
        }
        std::cout << " [freq=" << node->frequency << "]\n";
    }
    else
    {
        // Es un nodo interno
        std::cout << "INTERNAL [freq=" << node->frequency << "]\n";
    }

    // Recursión para los hijos
    if (node->left || node->right)
    {
        std::string new_prefix = prefix;
        new_prefix += (is_left ? "│   " : "    ");

        if (node->left)
        {
            print_node(node->left, new_prefix, true);
        }
        if (node->right)
        {
            print_node(node->right, new_prefix, false);
        }
    }
}

// huffman_tree.cpp

// Función pública: punto de entrada
std::map<Token, std::string, TokenCompare> HuffmanTree::generate_codes() const
{
    std::cout << "=== Generando códigos Huffman ===\n\n";

    std::map<Token, std::string, TokenCompare> codes;

    // Verificar que el árbol existe
    if (!root)
    {
        std::cerr << "Error: El árbol no ha sido construido\n";
        return codes;
    }

    // Caso especial: solo hay 1 token único
    if (root->is_leaf)
    {
        codes[root->token] = "0";
        std::cout << "Solo 1 token único, código asignado: \"0\"\n\n";
        return codes;
    }

    // Generar códigos recursivamente
    generate_codes_recursive(root, "", codes);

    // Mostrar códigos generados
    std::cout << "Códigos generados:\n";
    for (const auto &par : codes)
    {
        const Token &token = par.first;
        const std::string &code = par.second;

        std::cout << "  ";

        // Mostrar token
        if (token.type == LITERAL)
        {
            char c = (char)token.value;
            if (c >= 32 && c <= 126)
            {
                std::cout << "'" << c << "'";
            }
            else
            {
                std::cout << "ASCII " << token.value;
            }
        }
        else
        {
            std::cout << "REF(" << token.value << "," << token.distance << ")";
        }

        std::cout << " → \"" << code << "\" (" << code.length() << " bits)\n";
    }

    std::cout << "\n";

    return codes;
}

// Función recursiva privada: hace el trabajo real
void HuffmanTree::generate_codes_recursive(
    const std::shared_ptr<HuffmanNode> &node,
    const std::string &code,
    std::map<Token, std::string, TokenCompare> &codes) const
{

    // Caso base: llegamos a nullptr
    if (!node)
    {
        return;
    }

    // Caso base: es una hoja (tiene token)
    if (node->is_leaf)
    {
        codes[node->token] = code;
        return;
    }

    // Caso recursivo: es nodo interno
    // Ir a la izquierda agregando "0"
    if (node->left)
    {
        generate_codes_recursive(node->left, code + "0", codes);
    }

    // Ir a la derecha agregando "1"
    if (node->right)
    {
        generate_codes_recursive(node->right, code + "1", codes);
    }
}

// Imprimir el árbol completo
void HuffmanTree::print_tree() const
{
    std::cout << "=== Estructura del Árbol de Huffman ===\n\n";

    if (!root)
    {
        std::cout << "Árbol vacío\n";
        return;
    }

    std::cout << "ROOT [freq=" << root->frequency << "]\n";

    if (root->left)
    {
        print_node(root->left, "", true);
    }
    if (root->right)
    {
        print_node(root->right, "", false);
    }

    std::cout << "\nLeyenda:\n";
    std::cout << "  L = hijo izquierdo (bit 0)\n";
    std::cout << "  R = hijo derecho (bit 1)\n";
    std::cout << "  freq = frecuencia de aparición\n\n";
}