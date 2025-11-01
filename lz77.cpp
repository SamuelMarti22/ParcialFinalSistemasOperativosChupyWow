// lz77.cpp

#include "lz77.h"
#include <algorithm>

// ============================================================================
// CONSTRUCTOR Y MÉTODOS DE INICIALIZACIÓN
// ============================================================================

LZ77::LZ77() : cursor(0) {
    window.reserve(WINDOW_SIZE);
}

void LZ77::reset() {
    window.clear();
    cursor = 0;
}

// ============================================================================
// MÉTODOS PRIVADOS: BÚSQUEDA DE COINCIDENCIAS
// ============================================================================

Match LZ77::findLongestMatch(const uint8_t* input, size_t input_size) {
    Match best_match(0, 0);
    
    // Si no hay datos en la ventana o no hay más input, no hay coincidencias
    if (window.empty() || cursor >= input_size) {
        return best_match;
    }
    
    // Calcular cuántos bytes podemos mirar hacia adelante (lookahead buffer)
    size_t max_lookahead = std::min(
        static_cast<size_t>(LOOKAHEAD_SIZE),
        input_size - cursor
    );
    
    // Buscar en toda la ventana
    size_t window_size = window.size();
    
    for (size_t i = 0; i < window_size; ++i) {
        // Verificar cuántos caracteres consecutivos coinciden
        size_t match_length = 0;
        
        // IMPORTANTE: Permitir coincidencias que se solapan
        // Ejemplo: "aaaaa" donde la ventana tiene solo 1 'a' puede generar
        // una coincidencia de longitud 4 copiando desde 1 byte atrás repetidamente
        while (match_length < max_lookahead &&
               input[cursor + match_length] == window[i + (match_length % (window_size - i))]) {
            ++match_length;
        }
        
        // Si encontramos una coincidencia mejor, actualizarla
        if (match_length > best_match.length) {
            // Calcular la distancia: desde el final de la ventana hasta la posición de coincidencia
            best_match.position = static_cast<uint16_t>(window_size - i);
            best_match.length = static_cast<uint16_t>(match_length);
            
            // Optimización: si alcanzamos el máximo posible, terminar búsqueda
            if (match_length == max_lookahead) {
                break;
            }
        }
    }
    
    return best_match;
}

// ============================================================================
// MÉTODOS PRIVADOS: GESTIÓN DE LA VENTANA
// ============================================================================

void LZ77::updateWindow(const uint8_t* data, size_t length) {
    // Añadir los nuevos datos a la ventana
    for (size_t i = 0; i < length; ++i) {
        window.push_back(data[i]);
    }
    
    // Si la ventana excede el tamaño máximo, eliminar del inicio
    if (window.size() > WINDOW_SIZE) {
        size_t excess = window.size() - WINDOW_SIZE;
        window.erase(window.begin(), window.begin() + excess);
    }
}

// ============================================================================
// MÉTODOS PRIVADOS: LECTURA/ESCRITURA BINARIA
// ============================================================================

void LZ77::writeHeader(const LZ77Header& header, std::vector<uint8_t>& output) {
    uint8_t buffer[8];
    
    // Escribir num_tokens (4 bytes, little-endian)
    uint32ToLittleEndian(header.num_tokens, buffer);
    
    // Escribir original_size (4 bytes, little-endian)
    uint32ToLittleEndian(header.original_size, buffer + 4);
    
    // Añadir al output
    output.insert(output.end(), buffer, buffer + 8);
}

void LZ77::writeToken(const LZ77Token& token, std::vector<uint8_t>& output) {
    uint8_t buffer[5];
    
    // Byte 0: tipo
    buffer[0] = token.type;
    
    // Bytes 1-2: valor (little-endian)
    uint16ToLittleEndian(token.value, buffer + 1);
    
    // Bytes 3-4: distancia (little-endian)
    uint16ToLittleEndian(token.distance, buffer + 3);
    
    // Añadir al output
    output.insert(output.end(), buffer, buffer + 5);
}

LZ77Header LZ77::readHeader(const uint8_t* data) {
    LZ77Header header;
    
    // Leer num_tokens (bytes 0-3)
    header.num_tokens = littleEndianToUint32(data);
    
    // Leer original_size (bytes 4-7)
    header.original_size = littleEndianToUint32(data + 4);
    
    return header;
}

LZ77Token LZ77::readToken(const uint8_t* data) {
    LZ77Token token;
    
    // Byte 0: tipo
    token.type = data[0];
    
    // Bytes 1-2: valor
    token.value = littleEndianToUint16(data + 1);
    
    // Bytes 3-4: distancia
    token.distance = littleEndianToUint16(data + 3);
    
    return token;
}

// ============================================================================
// MÉTODO PÚBLICO: COMPRESIÓN
// ============================================================================

bool LZ77::compress(const uint8_t* input, size_t input_size, 
                    std::vector<uint8_t>& output) {
    // Validar entrada
    if (input == nullptr || input_size == 0) {
        return false;
    }
    
    // Reiniciar estado
    reset();
    
    // Vector temporal para almacenar los tokens
    std::vector<LZ77Token> tokens;
    tokens.reserve(input_size); // Reservar memoria para evitar realocaciones
    
    // Procesar el input según el formato especificado
    // Ejemplo: "hola hola" → [h][o][l][a][ ][REF(len=4,dist=5)]
    while (cursor < input_size) {
        // Buscar la coincidencia más larga en la ventana
        Match match = findLongestMatch(input, input_size);
        
        // Si encontramos una coincidencia válida (>= MIN_MATCH_LENGTH)
        if (match.isValid()) {
            // Generar token REFERENCE con (longitud, distancia)
            // distancia = cuántos bytes atrás está la coincidencia
            tokens.emplace_back(match.length, match.position);
            
            // Actualizar ventana con los bytes de la coincidencia
            updateWindow(input + cursor, match.length);
            
            // Avanzar cursor solo por la longitud de la coincidencia
            cursor += match.length;
            
        } else {
            // No hay coincidencia válida, generar token LITERAL
            uint8_t current_char = input[cursor];
            tokens.emplace_back(current_char);
            
            // Actualizar ventana con este carácter
            updateWindow(input + cursor, 1);
            
            // Avanzar cursor 1 posición
            cursor += 1;
        }
    }
    
    // Preparar el header
    LZ77Header header(
        static_cast<uint32_t>(tokens.size()),
        static_cast<uint32_t>(input_size)
    );
    
    // Escribir header (8 bytes)
    writeHeader(header, output);
    
    // Escribir todos los tokens (5 bytes cada uno)
    for (const auto& token : tokens) {
        writeToken(token, output);
    }
    
    return true;
}

// ============================================================================
// MÉTODO PÚBLICO: DESCOMPRESIÓN
// ============================================================================

bool LZ77::decompress(const uint8_t* input, size_t input_size,
                      std::vector<uint8_t>& output) {
    // Validar que al menos tenemos el header (8 bytes)
    if (input == nullptr || input_size < 8) {
        return false;
    }
    
    // Leer el header
    LZ77Header header = readHeader(input);
    
    // Validar que el tamaño del archivo es consistente
    size_t expected_size = 8 + (header.num_tokens * 5);
    if (input_size < expected_size) {
        return false;
    }
    
    // Reservar espacio para el output
    output.clear();
    output.reserve(header.original_size);
    
    // Procesar cada token
    const uint8_t* token_data = input + 8; // Saltar el header
    
    for (uint32_t i = 0; i < header.num_tokens; ++i) {
        LZ77Token token = readToken(token_data);
        token_data += 5; // Avanzar al siguiente token
        
        if (token.type == LITERAL) {
            // Token LITERAL: añadir el carácter directamente
            output.push_back(static_cast<uint8_t>(token.value));
            
        } else if (token.type == REFERENCE) {
            // Token REFERENCE: copiar desde posición anterior
            // distance indica cuántos bytes atrás está la coincidencia
            // value indica cuántos bytes copiar
            
            size_t current_pos = output.size();
            
            // Validar que la distancia es válida
            if (token.distance > current_pos) {
                return false; // Error: referencia inválida
            }
            
            // Posición desde donde copiar
            size_t copy_from = current_pos - token.distance;
            
            // Copiar byte por byte (importante para casos donde hay solapamiento)
            // Ejemplo: "aaaaa" → 'a' + REF(4,1) donde se copia desde 1 byte atrás
            // Esto permite copiar secuencias repetitivas que se expanden durante la copia
            for (uint16_t j = 0; j < token.value; ++j) {
                output.push_back(output[copy_from + j]);
            }
        } else {
            // Tipo de token desconocido
            return false;
        }
    }
    
    // Verificar que el tamaño descomprimido coincide con el esperado
    if (output.size() != header.original_size) {
        return false;
    }
    
    return true;
}

