#ifndef SHA256_H
#define SHA256_H

#include <cstdint>
#include <string>

class SHA256 {
public:
    SHA256();
    void update(const uint8_t* data, size_t length);
    void update(const std::string& data);
    void final(uint8_t digest[32]);
    
    // Función de conveniencia: hash directo
    static void hash(const uint8_t* data, size_t length, uint8_t digest[32]);
    static void hash(const std::string& data, uint8_t digest[32]);

private:
    void transform(const uint8_t* chunk);
    
    uint32_t state_[8];
    uint64_t count_;
    uint8_t buffer_[64];
};

#endif // SHA256_H
