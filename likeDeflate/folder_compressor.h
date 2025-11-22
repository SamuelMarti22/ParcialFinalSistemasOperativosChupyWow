#ifndef FOLDER_COMPRESSOR_H
#define FOLDER_COMPRESSOR_H

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

namespace FolderCompressor {

// Metadata de cada archivo dentro del paquete
struct FileEntry {
    std::string relative_path;  // ruta relativa desde la carpeta raíz
    uint64_t offset;            // posición en el buffer concatenado
    uint64_t size;              // tamaño original del archivo
    
    FileEntry() : offset(0), size(0) {}
    FileEntry(const std::string& path, uint64_t off, uint64_t sz)
        : relative_path(path), offset(off), size(sz) {}
};

// Header del archivo .chupydir
struct ChupyDirHeader {
    char magic[8];              // "CHUPYDIR"
    uint32_t version;           // versión del formato
    uint32_t num_files;         // cantidad de archivos en el paquete
    uint64_t total_uncompressed; // tamaño total sin comprimir
    uint64_t metadata_size;     // tamaño del bloque de metadata serializada
    
    ChupyDirHeader() {
        memcpy(magic, "CHUPYDIR", 8);
        version = 1;
        num_files = 0;
        total_uncompressed = 0;
        metadata_size = 0;
    }
    
    bool isValid() const {
        return memcmp(magic, "CHUPYDIR", 8) == 0;
    }
};

// Función principal: comprimir una carpeta completa
void compressFolder(const std::string& folder_path, const std::string& output_file);

// Función principal: descomprimir un archivo .chupydir
void decompressFolder(const std::string& input_file, const std::string& output_folder);

// Utilidades internas (públicas por si necesitas usarlas)
std::vector<uint8_t> serializeMetadata(const std::vector<FileEntry>& entries);
std::vector<FileEntry> deserializeMetadata(const uint8_t* data, size_t size);

} // namespace FolderCompressor

#endif // FOLDER_COMPRESSOR_H