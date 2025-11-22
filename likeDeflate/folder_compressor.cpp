#include "folder_compressor.h"
#include "lz77.h"
#include "huffman.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <cstring>
#include <mutex>
#include <omp.h>

namespace fs = std::filesystem;

namespace FolderCompressor {

static std::mutex critical_mutex;

static std::vector<uint8_t> readFileBinary(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("No se pudo leer: " + path);
    
    f.seekg(0, std::ios::end);
    size_t size = f.tellg();
    f.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    if (size > 0) {
        f.read(reinterpret_cast<char*>(buffer.data()), size);
    }
    return buffer;
}

static void writeFileBinary(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("No se pudo escribir: " + path);
    
    if (!data.empty()) {
        f.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
}

// Serialización de metadata
std::vector<uint8_t> serializeMetadata(const std::vector<FileEntry>& entries) {
    std::vector<uint8_t> buffer;
    
    for (const auto& entry : entries) {
        // Longitud del path (2 bytes)
        uint16_t path_len = static_cast<uint16_t>(entry.relative_path.size());
        buffer.push_back(path_len & 0xFF);
        buffer.push_back((path_len >> 8) & 0xFF);
        
        // Path (variable)
        buffer.insert(buffer.end(), 
                     entry.relative_path.begin(), 
                     entry.relative_path.end());
        
        // Offset (8 bytes)
        for (int i = 0; i < 8; i++) {
            buffer.push_back((entry.offset >> (i * 8)) & 0xFF);
        }
        
        // Size (8 bytes)
        for (int i = 0; i < 8; i++) {
            buffer.push_back((entry.size >> (i * 8)) & 0xFF);
        }
    }
    
    return buffer;
}

std::vector<FileEntry> deserializeMetadata(const uint8_t* data, size_t size) {
    std::vector<FileEntry> entries;
    size_t pos = 0;
    
    while (pos < size) {
        // Leer longitud del path
        if (pos + 2 > size) break;
        uint16_t path_len = data[pos] | (data[pos + 1] << 8);
        pos += 2;
        
        // Leer path
        if (pos + path_len > size) break;
        std::string path(reinterpret_cast<const char*>(data + pos), path_len);
        pos += path_len;
        
        // Leer offset
        if (pos + 8 > size) break;
        uint64_t offset = 0;
        for (int i = 0; i < 8; i++) {
            offset |= static_cast<uint64_t>(data[pos + i]) << (i * 8);
        }
        pos += 8;
        
        // Leer size
        if (pos + 8 > size) break;
        uint64_t file_size = 0;
        for (int i = 0; i < 8; i++) {
            file_size |= static_cast<uint64_t>(data[pos + i]) << (i * 8);
        }
        pos += 8;
        
        entries.emplace_back(path, offset, file_size);
    }
    
    return entries;
}

// Compresión de carpeta

void compressFolder(const std::string& folder_path, const std::string& output_file) {
    if (!fs::exists(folder_path) || !fs::is_directory(folder_path)) {
        throw std::runtime_error("La ruta no es una carpeta válida: " + folder_path);
    }
    
    // Recopilar todos los archivos recursivamente
    std::vector<std::string> file_paths;
    fs::path base_path(folder_path);
    
    for (const auto& entry : fs::recursive_directory_iterator(folder_path)) {
        if (entry.is_regular_file()) {
            file_paths.push_back(entry.path().string());
        }
    }
    
    if (file_paths.empty()) {
        throw std::runtime_error("No se encontraron archivos en la carpeta");
    }
    
    // Estructura para guardar datos de archivos
    struct FileData {
        std::string relative_path;
        std::vector<uint8_t> content;
        bool success;
        
        FileData() : success(false) {}
    };
    
    std::vector<FileData> file_data_vec(file_paths.size());
    
    // Uso de paralelización para leer archivos
    #pragma omp parallel for schedule(dynamic) default(none) shared(file_paths, file_data_vec, base_path)
    for (size_t i = 0; i < file_paths.size(); ++i) {
        FileData& fd = file_data_vec[i];
        
        try {
            fd.content = readFileBinary(file_paths[i]);
            fs::path relative = fs::relative(file_paths[i], base_path);
            fd.relative_path = relative.string();
            fd.success = true;
        } catch (...) {
            fd.success = false;
        }
    }
    
    // Concatenar archivos leídos exitosamente
    std::vector<FileEntry> file_entries;
    std::vector<uint8_t> concatenated_buffer;
    
    concatenated_buffer.reserve(file_paths.size() * 10240);
    
    for (const auto& fd : file_data_vec) {
        if (fd.success) {
            file_entries.emplace_back(
                fd.relative_path,
                concatenated_buffer.size(),
                fd.content.size()
            );
            
            concatenated_buffer.insert(
                concatenated_buffer.end(),
                fd.content.begin(),
                fd.content.end()
            );
        }
    }
    
    if (file_entries.empty()) {
        throw std::runtime_error("No se pudo leer ningún archivo");
    }
    
    // Comprimir con LZ77
    auto lz77_data = LZ77::compress(concatenated_buffer);
    
    // Aplicar Huffman sobre LZ77
    std::vector<uint32_t> symbols(lz77_data.begin(), lz77_data.end());
    auto huffman_data = huff::encodeHuffmanStream(symbols, 256, 15);
    
    // Serializar metadata
    auto metadata_bytes = serializeMetadata(file_entries);
    
    //Crear header
    ChupyDirHeader header;
    header.num_files = static_cast<uint32_t>(file_entries.size());
    header.total_uncompressed = static_cast<uint64_t>(concatenated_buffer.size());
    header.metadata_size = static_cast<uint64_t>(metadata_bytes.size());
    
    // Ensamblar archivo final
    std::vector<uint8_t> final_output;
    final_output.reserve(sizeof(header) + metadata_bytes.size() + huffman_data.size());
    
    final_output.insert(final_output.end(),
                       reinterpret_cast<uint8_t*>(&header),
                       reinterpret_cast<uint8_t*>(&header) + sizeof(header));
    
    final_output.insert(final_output.end(),
                       metadata_bytes.begin(),
                       metadata_bytes.end());
    
    final_output.insert(final_output.end(),
                       huffman_data.begin(),
                       huffman_data.end());
    
    //Guardar archivo
    writeFileBinary(output_file, final_output);
}

// Descompresión de carpeta

void decompressFolder(const std::string& input_file, const std::string& output_folder) {
    // Leer archivo completo
    auto file_data = readFileBinary(input_file);
    
    if (file_data.size() < sizeof(ChupyDirHeader)) {
        throw std::runtime_error("Archivo demasiado pequeño o corrupto");
    }
    
    // Leer header
    ChupyDirHeader header;
    std::memcpy(&header, file_data.data(), sizeof(header));
    
    if (!header.isValid()) {
        throw std::runtime_error("No es un archivo .chupydir válido");
    }
    
    // Leer metadata
    size_t metadata_start = sizeof(ChupyDirHeader);
    size_t metadata_end = metadata_start + header.metadata_size;
    
    if (metadata_end > file_data.size()) {
        throw std::runtime_error("Metadata corrupta o truncada");
    }
    
    auto file_entries = deserializeMetadata(
        file_data.data() + metadata_start,
        header.metadata_size
    );
    
    if (file_entries.size() != header.num_files) {
        throw std::runtime_error("Número de archivos no coincide con el header");
    }
    
    // Descomprimir datos con Huffman
    const uint8_t* compressed_start = file_data.data() + metadata_end;
    size_t compressed_size = file_data.size() - metadata_end;
    
    auto symbols = huff::decodeHuffmanStream(compressed_start, compressed_size);
    std::vector<uint8_t> lz77_data(symbols.begin(), symbols.end());
    
    // Descomprimir LZ77
    auto decompressed = LZ77::decompress(lz77_data);
    
    if (decompressed.size() != header.total_uncompressed) {
        throw std::runtime_error("Tamaño descomprimido no coincide");
    }
    
    // Crear carpeta de salida
    fs::create_directories(output_folder);
    
    // Paralelización al extraer archivos en paralelo
    #pragma omp parallel for schedule(dynamic) default(none) shared(file_entries, output_folder, decompressed)
    for (size_t i = 0; i < file_entries.size(); ++i) {
        const auto& entry = file_entries[i];
        
        try {
            fs::path output_path = fs::path(output_folder) / entry.relative_path;
            
            // Crear subdirectorios si es necesario
            fs::create_directories(output_path.parent_path());
            
            // Extraer datos del archivo
            if (entry.offset + entry.size <= decompressed.size()) {
                std::vector<uint8_t> file_data(
                    decompressed.begin() + entry.offset,
                    decompressed.begin() + entry.offset + entry.size
                );
                
                // Escribir archivo
                writeFileBinary(output_path.string(), file_data);
            }
        } catch (...) {
            // Ignorar errores en archivos individuales
        }
    }
}

} 