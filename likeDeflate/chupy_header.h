#ifndef CHUPY_HEADER_H
#define CHUPY_HEADER_H

#include <cstdint>
#include <vector>
#include <string>

namespace chupy {

// Estructura del header del archivo .chupy
// Total: 25 bytes
struct ChupyHeader {
    char magic[8];           // "CHUPY\0\0\0"
    uint16_t version;        // versión del formato (actualmente 1)
    uint8_t ext_len;         // longitud de la extensión
    char extension[16];      // extensión original (ej: ".txt", ".jpg")
    
    ChupyHeader();
    
    // Configurar extensión (trunca a 15 chars si es necesario)
    void setExtension(const std::string& ext);
    
    // Obtener extensión como string
    std::string getExtension() const;
    
    // Validar magic number y versión
    bool isValid() const;
    
    // Serializar a bytes
    std::vector<uint8_t> serialize() const;
    
    // Deserializar desde bytes
    static ChupyHeader deserialize(const uint8_t* data);
};

// Estructura para leer archivo .chupy completo
struct ChupyFile {
    ChupyHeader header;
    std::vector<uint8_t> compressed_data;
    bool valid;
};

// Crear archivo .chupy completo (header + datos comprimidos)
std::vector<uint8_t> createChupyFile(
    const std::string& original_extension,
    const std::vector<uint8_t>& compressed_data
);

// Leer archivo .chupy y extraer header y datos
ChupyFile readChupyFile(const std::vector<uint8_t>& file_data);

} // namespace chupy

#endif