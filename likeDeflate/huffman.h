#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>

namespace huff {

class BitWriter {
public:
    void writeBits(uint32_t bits, int nbits) {
        while (nbits > 0) {
            int space = 8 - bitpos_;
            int take = (nbits < space) ? nbits : space;
            uint32_t chunk = bits & ((1u << take) - 1u);
            current_ |= (uint8_t)(chunk << bitpos_);
            bitpos_ += take;
            bits >>= take;
            nbits -= take;
            if (bitpos_ == 8) {
                out_.push_back(current_);
                current_ = 0;
                bitpos_ = 0;
            }
        }
    }
    void flushZeroPadding() {
        if (bitpos_ != 0) {
            out_.push_back(current_);
            current_ = 0;
            bitpos_ = 0;
        }
    }
    const std::vector<uint8_t>& data() const { return out_; }
    std::vector<uint8_t>& data() { return out_; }
private:
    std::vector<uint8_t> out_;
    uint8_t current_ = 0;
    int bitpos_ = 0;
};

class BitReader {
public:
    BitReader(const uint8_t* data, size_t size) : data_(data), size_(size) {}
    uint32_t readBits(int nbits) {
        uint32_t result = 0;
        int filled = 0;
        while (filled < nbits) {
            if (bitpos_ == 8) { bitpos_ = 0; ++idx_; }
            if (idx_ >= size_) throw std::runtime_error("BitReader: out of data");
            int avail = 8 - bitpos_;
            int take = (nbits - filled < avail) ? (nbits - filled) : avail;
            uint32_t bits = (data_[idx_] >> bitpos_) & ((1u << take) - 1u);
            result |= (bits << filled);
            bitpos_ += take;
            filled += take;
            if (bitpos_ == 8) { bitpos_ = 0; ++idx_; }
        }
        return result;
    }
    void alignToByte() { if (bitpos_ != 0) { bitpos_ = 0; ++idx_; } }
    size_t bytesConsumed() const { return idx_; }
private:
    const uint8_t* data_;
    size_t size_;
    size_t idx_ = 0;
    int bitpos_ = 0;
};

// ---------------- Huffman canónico ----------------

struct Code {
    uint32_t code = 0; // MSB-first canónico (sin revertir)
    uint8_t  len  = 0;
};

class CanonicalHuffman {
public:
    // Construye a partir de frecuencias (alphabetSize = frequencies.size()).
    // maxCodeLen típico 15.
    void build(const std::vector<uint32_t>& frequencies, uint8_t maxCodeLen = 15);

    // Reconstruye desde longitudes (códigos canónicos deterministas).
    void loadFromCodeLengths(const std::vector<uint8_t>& codeLengths);

    // Codifica un símbolo (escribe los bits en orden LSB-first).
    void encodeSymbol(BitWriter& bw, uint32_t sym) const;

    // Decodifica un símbolo leyendo bits LSB-first.
    uint32_t decodeSymbol(BitReader& br) const;

    const std::vector<uint8_t>& codeLengths() const { return codeLen_; }
    const std::vector<Code>& codes() const { return codes_; }

private:
    void buildDecoder(); // prepara tablas internas

    std::vector<uint8_t>  codeLen_;
    std::vector<Code>     codes_;

    // Tablas para decoder LSB-first (simples y seguras)
    std::vector<uint32_t>              revCode_;    // código bit-revertido por símbolo
    std::vector<std::vector<uint32_t>> byLenSyms_;  // símbolos agrupados por longitud
};

// -------------- Stream simple (cabecera + bitstream) --------------
// Formato:
// [u16 alphabet_size][alphabet_size bytes: code_len][u32 num_symbols][payload bits LSB-first]
std::vector<uint8_t> encodeHuffmanStream(const std::vector<uint32_t>& symbols,
                                         uint32_t alphabetSize,
                                         uint8_t maxCodeLen = 15);

std::vector<uint32_t> decodeHuffmanStream(const uint8_t* data, size_t size);

} // namespace huff
