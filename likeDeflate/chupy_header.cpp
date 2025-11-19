#include "chupy_header.h"
#include <cstring>
#include <algorithm>

namespace chupy {

ChupyHeader::ChupyHeader() {
    std::memset(this, 0, sizeof(ChupyHeader));
    std::memcpy(magic, "CHUPY", 5);
    version = 1;
}

void ChupyHeader::setExtension(const std::string& ext) {
    ext_len = static_cast<uint8_t>(std::min(size_t(15), ext.length()));
    std::memcpy(extension, ext.c_str(), ext_len);
    extension[ext_len] = '\0';
}

std::string ChupyHeader::getExtension() const {
    return std::string(extension, ext_len);
}

bool ChupyHeader::isValid() const {
    return std::memcmp(magic, "CHUPY", 5) == 0 && version == 1;
}

std::vector<uint8_t> ChupyHeader::serialize() const {
    std::vector<uint8_t> data(sizeof(ChupyHeader));
    std::memcpy(data.data(), this, sizeof(ChupyHeader));
    return data;
}

ChupyHeader ChupyHeader::deserialize(const uint8_t* data) {
    ChupyHeader header;
    std::memcpy(&header, data, sizeof(ChupyHeader));
    return header;
}

std::vector<uint8_t> createChupyFile(
    const std::string& original_extension,
    const std::vector<uint8_t>& compressed_data)
{
    // Crear header
    ChupyHeader header;
    header.setExtension(original_extension);
    
    // Serializar header
    auto header_data = header.serialize();
    
    // Combinar header + datos comprimidos
    std::vector<uint8_t> result;
    result.reserve(header_data.size() + compressed_data.size());
    result.insert(result.end(), header_data.begin(), header_data.end());
    result.insert(result.end(), compressed_data.begin(), compressed_data.end());
    
    return result;
}

ChupyFile readChupyFile(const std::vector<uint8_t>& file_data) {
    ChupyFile result;
    result.valid = false;
    
    // Verificar tamaño mínimo
    if (file_data.size() < sizeof(ChupyHeader)) {
        return result;
    }
    
    // Deserializar header
    result.header = ChupyHeader::deserialize(file_data.data());
    
    // Validar header
    if (!result.header.isValid()) {
        return result;
    }
    
    // Extraer datos comprimidos
    const size_t header_size = sizeof(ChupyHeader);
    result.compressed_data.assign(
        file_data.begin() + header_size,
        file_data.end()
    );
    
    result.valid = true;
    return result;
}

} // namespace chupy