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
#include "chupy_header.h"

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
    cout << "\nEstadisticas Compresion\n";
    cout << std::fixed << std::setprecision(2);
    cout << "Original:   " << orig << " bytes  (" << std::setw(6) << 100.00           << "%)\n";
    cout << "LZ77:       " << lzsz << " bytes  (" << std::setw(6) << pct(lzsz, orig)  << "%)\n";
    cout << "Huffman - comprimido: " << compsz << " bytes  (" << std::setw(6) << pct(compsz, orig) << "%)\n";

}

// ------------------------- compresión -------------------------

static void do_compress(const std::string &inPath, const std::string &outPath)
{
    // 1) Leer original
    auto input = readFile(inPath);
    
    // Extraer la extensión original
    fs::path p(inPath);
    std::string original_ext = p.extension().string();

    //LZ77
    std::vector<uint8_t> lz77_bytes = LZ77::compress(input);

    // Huffman sobre stream LZ77 
    std::vector<uint32_t> syms(lz77_bytes.begin(), lz77_bytes.end());
    auto huff_blob = encodeHuffmanStream(syms, /*alphabetSize=*/256, /*maxCodeLen=*/15);

    // 4) Crear archivo .chupy con header (SOLO extensión)
    auto final_output = chupy::createChupyFile(
        original_ext,
        huff_blob
    );

    // Escribir .chupy
    writeFile(outPath, final_output);

    // Verificación rápida en memoria + stats - CAMBIO: llamada estática directa
    auto back_syms = decodeHuffmanStream(huff_blob.data(), huff_blob.size());
    std::vector<uint8_t> lz77_back(back_syms.begin(), back_syms.end());

    std::vector<uint8_t> restored = LZ77::decompress(lz77_back);

    if (restored != input) {
        std::cerr << "La verificación de integridad falló\n";
    }

    print_stats(input.size(), lz77_bytes.size(), final_output.size(), restored.size());
}

// ------------------------- descompresión -------------------------

static void do_decompress(const std::string &inPath, const std::string &outPath)
{
    auto blob = readFile(inPath);
    std::cout << "Leídos " << blob.size() << " bytes de " << inPath << "\n";
    
    // Leer y validar archivo .chupy
    auto chupy_file = chupy::readChupyFile(blob);
    
    if (!chupy_file.valid) {
        throw std::runtime_error("Archivo no es un .chupy válido");
    }
    
    const auto& header = chupy_file.header;
    
    
    // Decodificar Huffman
    auto syms = decodeHuffmanStream(
        chupy_file.compressed_data.data(),
        chupy_file.compressed_data.size()
    );
    std::vector<uint8_t> lz77_bytes(syms.begin(), syms.end());
    std::cout << "Huffman decodificó " << lz77_bytes.size() << " bytes\n";

    // Descomprimir LZ77 - CAMBIO: llamada estática directa
    std::vector<uint8_t> restored = LZ77::decompress(lz77_bytes);

    // Determinar nombre de salida automático
    std::string final_output_path = outPath;
    
    if (final_output_path.empty()) {
        // Generar automáticamente: archivo_restored.ext
        fs::path p(inPath);
        final_output_path = p.stem().string() + "_restored" + header.getExtension();
    } else {
        // Si el usuario dio un path pero sin extensión, agregar la original
        fs::path p(final_output_path);
        if (p.extension().empty() && !header.getExtension().empty()) {
            final_output_path += header.getExtension();
        }
    }
    
    // Escribir archivo restaurado
    writeFile(final_output_path, restored);
    std::cout << "Restaurado en " << final_output_path << " (" << restored.size() << " bytes)\n";
    std::cout << "✓ Descompresión completada\n";
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
//mientras para que permita tener 2 mains
int menu_standalone()   
{
    try {
        while (true) {
            std::cout << "\n=== Menu LZ77 + Huffman (formato .chupy) ===\n"
                      << "1) Comprimir archivo\n"
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
                std::cout << "Ruta de salida (Enter para automático): ";
                std::cin.ignore();
                std::getline(std::cin, outPath);
                
                do_decompress(inPath, outPath);
                break;
            }
            default:
                std::cout << "Opción inválida.\n";
                break;
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "\n⌘ Error: " << e.what() << "\n";
        return 1;
    }
}