// main_lz77_huffman_menu.cpp (sin header, salida .chupy)

#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <filesystem>
namespace fs = std::filesystem;

#include "lz77.h"    // tu implementación (LZ77::compress / decompress que devuelven vector)
#include "huffman.h" // namespace huff, con encodeHuffmanStream / decodeHuffmanStream

using namespace huff;

// ------------------------- utilidades de archivo -------------------------

static std::vector<uint8_t> readFile(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("No pude abrir: " + path);
    f.seekg(0, std::ios::end);
    std::streamoff sz = f.tellg();
    f.seekg(0, std::ios::beg);
    if (sz < 0)
        throw std::runtime_error("Error leyendo tamaño: " + path);
    std::vector<uint8_t> buf((size_t)sz);
    if (sz > 0)
        f.read(reinterpret_cast<char *>(buf.data()), sz);
    return buf;
}

static void writeFile(const std::string &path, const std::vector<uint8_t> &data)
{
    std::ofstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("No pude crear: " + path);
    if (!data.empty())
        f.write(reinterpret_cast<const char *>(data.data()), (std::streamsize)data.size());
}

// ------------------------- utilidades de impresión -------------------------

static inline double pct(std::size_t part, std::size_t whole)
{
    return whole == 0 ? 0.0 : 100.0 * (double)part / (double)whole;
}

static void print_stats(std::size_t orig,
                        std::size_t lzsz,
                        std::size_t compsz,
                        std::size_t recov)
{
    using std::cout;
    cout << "\n── ESTADÍSTICAS FINALES ──\n";
    cout << std::fixed << std::setprecision(2);
    cout << "Original:   " << std::setw(8) << orig << " bytes  (" << std::setw(6) << 100.00           << "%)\n";
    cout << "LZ77:       " << std::setw(8) << lzsz << " bytes  (" << std::setw(6) << pct(lzsz, orig)  << "%)\n";
    cout << "Comprimido: " << std::setw(8) << compsz << " bytes  (" << std::setw(6) << pct(compsz, orig) << "%)\n";
    cout << "Recuperado: " << std::setw(8) << recov << " bytes\n";
}

// ------------------------- compresión -------------------------

static void do_compress(const std::string &inPath, const std::string &outPath)
{
    // 1) Leer original
    auto input = readFile(inPath);
    std::cout << "Leídos " << input.size() << " bytes de " << inPath << "\n";

    // 2) LZ77
    LZ77 lz;
    std::vector<uint8_t> lz77_bytes = lz.compress(input);
    std::cout << "LZ77 produjo " << lz77_bytes.size() << " bytes\n";

    // 3) Huffman sobre stream LZ77 (alfabeto 0..255)
    std::vector<uint32_t> syms(lz77_bytes.begin(), lz77_bytes.end());
    auto huff_blob = encodeHuffmanStream(syms, /*alphabetSize=*/256, /*maxCodeLen=*/15);

    // 4) Escribir .chupy
    writeFile(outPath, huff_blob);
    std::cout << "Escrito " << outPath << " (" << huff_blob.size() << " bytes)\n";

    // 5) Verificación rápida en memoria + stats
    auto back_syms = decodeHuffmanStream(huff_blob.data(), huff_blob.size());
    std::vector<uint8_t> lz77_back(back_syms.begin(), back_syms.end());

    LZ77 lz2;
    std::vector<uint8_t> restored = lz2.decompress(lz77_back);

    if (restored != input) {
        std::cerr << "AVISO: la verificación de integridad FALLÓ\n";
    }

    print_stats(input.size(), lz77_bytes.size(), huff_blob.size(), restored.size());
}

// ------------------------- descompresión -------------------------

static void do_decompress(const std::string &inPath, const std::string &outPath)
{
    auto blob = readFile(inPath);
    std::cout << "Leídos " << blob.size() << " bytes de " << inPath << "\n";

    auto syms = decodeHuffmanStream(blob.data(), blob.size());
    std::vector<uint8_t> lz77_bytes(syms.begin(), syms.end());
    std::cout << "Huffman decodificó " << lz77_bytes.size() << " bytes (stream LZ77)\n";

    LZ77 lz;
    std::vector<uint8_t> restored = lz.decompress(lz77_bytes);

    writeFile(outPath, restored);
    std::cout << "Restaurado en " << outPath << " (" << restored.size() << " bytes)\n";
}
// ------------------------- interfaz pública temporal  -------------------------

void comprimirConDeflate(const std::string& archivoEntrada, const std::string& archivoSalida) {
    const std::string salidaFinal = fs::path(archivoSalida).replace_extension(".chupy").string();
    do_compress(archivoEntrada, salidaFinal);
}

void descomprimirConDeflate(const std::string& archivoEntrada, const std::string& archivoSalida) {
    do_decompress(archivoEntrada, archivoSalida);
}

// ------------------------- menú principal -------------------------

int main()
{
    try {
        while (true) {
            std::cout << "\n=== Menu LZ77 + Huffman ===\n"
                      << "1) Comprimir archivo (con verificación y estadísticas)\n"
                      << "2) Descomprimir archivo\n"
                      << "0) Salir\n"
                      << "Selección: ";
            int op = -1;
            if (!(std::cin >> op))
                return 0;
            if (op == 0)
                return 0;

            std::string inPath, outPath;
            switch (op) {
            case 1: {
                std::cout << "Ruta del archivo a comprimir: ";
                std::cin >> inPath;
                const std::string forcedOut =
                    fs::path(inPath).replace_extension(".chupy").string();
                do_compress(inPath, forcedOut);
                break;
            }
            case 2: {
                std::cout << "Ruta del archivo .chupy: ";
                std::cin >> inPath;
                std::cout << "Ruta de salida del archivo restaurado: ";
                std::cin >> outPath;
                do_decompress(inPath, outPath);
                break;
            }
            default:
                std::cout << "Opción inválida.\n";
                break;
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
