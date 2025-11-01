/**
 * @file lz77.h
 * @brief Implementación del algoritmo de compresión LZ77
 * 
 * Este archivo define las estructuras y funciones necesarias para comprimir
 * y descomprimir datos usando el algoritmo LZ77 con un formato binario específico.
 * 
 * FORMATO DE SALIDA:
 * [HEADER: 8 bytes] [TOKEN_1: 5 bytes] [TOKEN_2: 5 bytes] ... [TOKEN_N: 5 bytes]
 */

#ifndef LZ77_H
#define LZ77_H

#include <cstdint>
#include <vector>
#include <string>

// ============================================================================
// CONSTANTES Y CONFIGURACIÓN
// ============================================================================

/**
 * @brief Tamaño de la ventana deslizante (diccionario)
 * 
 * Define cuántos bytes hacia atrás puede buscar el algoritmo para
 * encontrar coincidencias. El valor máximo es 65535 (uint16_t).
 * Un valor típico es 4096 bytes (4KB).
 */
constexpr uint16_t WINDOW_SIZE = 4096;

/**
 * @brief Tamaño del buffer de lookahead
 * 
 * Define cuántos bytes hacia adelante puede leer el algoritmo para
 * encontrar la coincidencia más larga. El valor máximo es 65535 (uint16_t).
 * Un valor típico es 18 bytes.
 */
constexpr uint16_t LOOKAHEAD_SIZE = 18;

/**
 * @brief Longitud mínima para considerar una coincidencia
 * 
 * Si una coincidencia es menor a este valor, es más eficiente usar
 * un token LITERAL en lugar de REFERENCE (porque cada token ocupa 5 bytes).
 */
constexpr uint16_t MIN_MATCH_LENGTH = 3;

// ============================================================================
// ESTRUCTURAS DE DATOS
// ============================================================================

/**
 * @brief Tipo de token en la salida comprimida
 */
enum TokenType : uint8_t {
    LITERAL = 0,    ///< Carácter sin comprimir
    REFERENCE = 1   ///< Referencia a texto anterior
};

/**
 * @brief Estructura del Header (8 bytes)
 * 
 * Contiene metadatos sobre el archivo comprimido.
 */
struct LZ77Header {
    uint32_t num_tokens;      ///< Número total de tokens (byte 0-3)
    uint32_t original_size;   ///< Tamaño del archivo original en bytes (byte 4-7)
    
    LZ77Header() : num_tokens(0), original_size(0) {}
    LZ77Header(uint32_t tokens, uint32_t size) 
        : num_tokens(tokens), original_size(size) {}
};

/**
 * @brief Estructura de un Token (5 bytes)
 * 
 * Representa un elemento en la salida comprimida.
 * Puede ser un LITERAL (carácter sin comprimir) o una REFERENCE (coincidencia).
 */
struct LZ77Token {
    uint8_t type;        ///< Tipo: LITERAL (0) o REFERENCE (1) - byte 0
    uint16_t value;      ///< Valor: ASCII si LITERAL, longitud si REFERENCE - byte 1-2
    uint16_t distance;   ///< Distancia: 0 si LITERAL, posición relativa si REFERENCE - byte 3-4
    
    /**
     * @brief Constructor para token LITERAL
     * @param character Carácter ASCII a almacenar
     */
    LZ77Token(uint8_t character) 
        : type(LITERAL), value(character), distance(0) {}
    
    /**
     * @brief Constructor para token REFERENCE
     * @param match_length Longitud de la coincidencia (n)
     * @param match_distance Distancia hacia atrás de la coincidencia (p)
     */
    LZ77Token(uint16_t match_length, uint16_t match_distance)
        : type(REFERENCE), value(match_length), distance(match_distance) {}
    
    /**
     * @brief Constructor por defecto
     */
    LZ77Token() : type(LITERAL), value(0), distance(0) {}
};

/**
 * @brief Estructura para representar una coincidencia encontrada
 * 
 * Esta estructura es usada internamente durante el proceso de compresión
 * para almacenar información sobre la mejor coincidencia encontrada.
 */
struct Match {
    uint16_t position;  ///< Posición de la coincidencia en la ventana (p)
    uint16_t length;    ///< Longitud de la coincidencia (n)
    
    Match() : position(0), length(0) {}
    Match(uint16_t pos, uint16_t len) : position(pos), length(len) {}
    
    /**
     * @brief Verifica si la coincidencia es válida
     * @return true si la longitud es >= MIN_MATCH_LENGTH
     */
    bool isValid() const {
        return length >= MIN_MATCH_LENGTH;
    }
};

// ============================================================================
// CLASE PRINCIPAL LZ77
// ============================================================================

/**
 * @brief Clase que implementa el algoritmo de compresión/descompresión LZ77
 */
class LZ77 {
private:
    std::vector<uint8_t> window;      ///< Ventana deslizante (diccionario)
    size_t cursor;                     ///< Posición actual en el buffer de entrada
    
    /**
     * @brief Busca la coincidencia más larga en la ventana
     * 
     * Implementa la lógica del pseudocódigo:
     * "prefix := longest prefix of input that begins in window"
     * 
     * @param input Buffer de entrada completo
     * @param input_size Tamaño del buffer de entrada
     * @return Match con la posición (p) y longitud (n) de la mejor coincidencia
     */
    Match findLongestMatch(const uint8_t* input, size_t input_size);
    
    /**
     * @brief Actualiza la ventana deslizante
     * 
     * Implementa la lógica del pseudocódigo:
     * "discard l + 1 chars from front of window"
     * "append s to back of window"
     * 
     * @param data Datos a añadir a la ventana
     * @param length Cantidad de bytes a añadir
     */
    void updateWindow(const uint8_t* data, size_t length);
    
    /**
     * @brief Escribe el header en formato little-endian
     * @param header Estructura del header a escribir
     * @param output Vector donde se escribirán los bytes
     */
    void writeHeader(const LZ77Header& header, std::vector<uint8_t>& output);
    
    /**
     * @brief Escribe un token en formato little-endian
     * @param token Token a escribir
     * @param output Vector donde se escribirán los bytes
     */
    void writeToken(const LZ77Token& token, std::vector<uint8_t>& output);
    
    /**
     * @brief Lee el header desde datos binarios
     * @param data Puntero a los datos binarios
     * @return Header leído
     */
    LZ77Header readHeader(const uint8_t* data);
    
    /**
     * @brief Lee un token desde datos binarios
     * @param data Puntero a los datos binarios (5 bytes)
     * @return Token leído
     */
    LZ77Token readToken(const uint8_t* data);

public:
    /**
     * @brief Constructor
     */
    LZ77();
    
    /**
     * @brief Comprime datos usando LZ77
     * - Busca el prefijo más largo que coincida en la ventana
     * - Si existe coincidencia válida (>= MIN_MATCH_LENGTH):
     *   → Genera token REFERENCE (longitud, distancia)
     * - Si no existe coincidencia:
     *   → Genera token LITERAL (carácter)
     * - Actualiza ventana y cursor
     * 
     * Formato de salida:
     * [HEADER: 8 bytes] [TOKEN_1: 5 bytes] [TOKEN_2: 5 bytes] ... [TOKEN_N: 5 bytes]
     * - Cada token es independiente (LITERAL o REFERENCE)
     * - Los tokens REFERENCE no incluyen el siguiente carácter
     * 
     * Ejemplo: "hola hola" → [h][o][l][a][ ][REF(len=4,dist=5)]
     * 
     * @param input Puntero a los datos de entrada
     * @param input_size Tamaño de los datos de entrada
     * @param output Vector donde se almacenarán los datos comprimidos
     * @return true si la compresión fue exitosa
     */
    bool compress(const uint8_t* input, size_t input_size, 
                  std::vector<uint8_t>& output);
    
    /**
     * @brief Descomprime datos comprimidos con LZ77
     * 
     * Lee el header y los tokens, reconstruyendo el archivo original:
     * - LITERAL: añade el carácter directamente
     * - REFERENCE: copia 'length' bytes desde 'distance' posiciones atrás
     * 
     * @param input Puntero a los datos comprimidos
     * @param input_size Tamaño de los datos comprimidos
     * @param output Vector donde se almacenarán los datos descomprimidos
     * @return true si la descompresión fue exitosa
     */
    bool decompress(const uint8_t* input, size_t input_size,
                    std::vector<uint8_t>& output);
    
    /**
     * @brief Reinicia el estado del compresor
     */
    void reset();
};

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

/**
 * @brief Convierte uint16_t a little-endian
 * @param value Valor a convertir
 * @param buffer Buffer donde escribir los 2 bytes
 */
inline void uint16ToLittleEndian(uint16_t value, uint8_t* buffer) {
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
}

/**
 * @brief Convierte uint32_t a little-endian
 * @param value Valor a convertir
 * @param buffer Buffer donde escribir los 4 bytes
 */
inline void uint32ToLittleEndian(uint32_t value, uint8_t* buffer) {
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = (value >> 16) & 0xFF;
    buffer[3] = (value >> 24) & 0xFF;
}

/**
 * @brief Lee uint16_t desde little-endian
 * @param buffer Buffer con 2 bytes en little-endian
 * @return Valor convertido
 */
inline uint16_t littleEndianToUint16(const uint8_t* buffer) {
    return static_cast<uint16_t>(buffer[0]) | 
           (static_cast<uint16_t>(buffer[1]) << 8);
}

/**
 * @brief Lee uint32_t desde little-endian
 * @param buffer Buffer con 4 bytes en little-endian
 * @return Valor convertido
 */
inline uint32_t littleEndianToUint32(const uint8_t* buffer) {
    return static_cast<uint32_t>(buffer[0]) | 
           (static_cast<uint32_t>(buffer[1]) << 8) |
           (static_cast<uint32_t>(buffer[2]) << 16) |
           (static_cast<uint32_t>(buffer[3]) << 24);
}

#endif // LZ77_H