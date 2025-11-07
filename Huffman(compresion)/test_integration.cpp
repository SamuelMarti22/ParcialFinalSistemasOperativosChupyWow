// test_full_pipeline.cpp - Versión interactiva
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring> 
#include "lz77.h"
#include "huffman_compressor.h"
#include "huffman_decompressor.h"
#include "common.h"

using namespace std;

int main() {
    cout << "╔════════════════════════════════════════════════════════════╗\n";
    cout << "║         TEST PIPELINE COMPLETO: LZ77 + HUFFMAN            ║\n";
    cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    // ══════════════════════════════════════════════════════════════
    // SOLICITAR NOMBRE DEL ARCHIVO
    // ══════════════════════════════════════════════════════════════
    string filename;
    cout << "Ingrese el nombre del archivo a comprimir: ";
    getline(cin, filename);
    
    // Intentar abrir el archivo
    ifstream test_file(filename, ios::binary);
    if (!test_file.is_open()) {
        cout << "✗ Error: No se pudo abrir el archivo '" << filename << "'\n";
        cout << "  Verifique que el archivo existe y tiene permisos de lectura.\n";
        return 1;
    }
    
    // Leer el archivo completo
    vector<uint8_t> original((istreambuf_iterator<char>(test_file)),
                             istreambuf_iterator<char>());
    test_file.close();
    
    cout << "\n✓ Archivo leído correctamente\n";
    cout << "  Nombre: " << filename << "\n";
    cout << "  Tamaño: " << original.size() << " bytes\n\n";
    
    // Mostrar preview del contenido (primeros 50 caracteres)
    if (original.size() > 0) {
        cout << "  Preview: \"";
        size_t preview_len = min(size_t(50), original.size());
        for (size_t i = 0; i < preview_len; i++) {
            char c = original[i];
            if (c >= 32 && c <= 126) {
                cout << c;
            } else if (c == '\n') {
                cout << "\\n";
            } else if (c == '\t') {
                cout << "\\t";
            } else {
                cout << ".";
            }
        }
        if (original.size() > 50) cout << "...";
        cout << "\"\n\n";
    }
    
    // ══════════════════════════════════════════════════════════════
    // PASO 1: COMPRIMIR CON LZ77
    // ══════════════════════════════════════════════════════════════
    cout << "═══ PASO 1: Compresión LZ77 ═══\n";
    
    LZ77 lz77_comp;
    vector<uint8_t> lz77_data;
    
    if (!lz77_comp.compress(original.data(), original.size(), lz77_data)) {
        cout << "✗ Error en compresión LZ77\n";
        return 1;
    }
    
    // Guardar .lz77
    string lz77_filename = filename + ".lz77";
    ofstream lz77_file(lz77_filename, ios::binary);
    lz77_file.write((const char*)lz77_data.data(), lz77_data.size());
    lz77_file.close();
    
    cout << "✓ LZ77: " << lz77_data.size() << " bytes\n";
    cout << "✓ Guardado: " << lz77_filename << "\n\n";
    
    // Mostrar header LZ77
    uint32_t num_tokens_lz77 = *(uint32_t*)lz77_data.data();
    uint32_t original_size_lz77 = *(uint32_t*)(lz77_data.data() + 4);
    
    cout << "Header LZ77 original:\n";
    cout << "  num_tokens: " << num_tokens_lz77 << "\n";
    cout << "  original_size: " << original_size_lz77 << "\n";
    double ratio_lz77 = 100.0 * lz77_data.size() / original.size();
    cout << "  Ratio LZ77: " << ratio_lz77 << "%\n\n";
    
    // ══════════════════════════════════════════════════════════════
    // PASO 2: COMPRIMIR CON HUFFMAN
    // ══════════════════════════════════════════════════════════════
    cout << "═══ PASO 2: Compresión Huffman ═══\n";
    
    string huff_filename = filename + ".compressed";
    HuffmanCompressor huff_comp;
    
    if (!huff_comp.compress_file(lz77_filename, huff_filename)) {
        cout << "✗ Error en compresión Huffman\n";
        return 1;
    }
    
    // Obtener tamaño del archivo .huff
    ifstream huff_size_check(huff_filename, ios::binary | ios::ate);
    size_t huff_size = huff_size_check.tellg();
    huff_size_check.close();
    
    cout << "\n✓ Huffman completado\n";
    cout << "✓ Guardado: " << huff_filename << "\n";
    cout << "  Tamaño: " << huff_size << " bytes\n\n";
    
    // ══════════════════════════════════════════════════════════════
    // PASO 3: DESCOMPRIMIR HUFFMAN
    // ══════════════════════════════════════════════════════════════
    cout << "═══ PASO 3: Descompresión Huffman ═══\n";
    
    string lz77_recovered_filename = filename + ".recovered.lz77";
    HuffmanDecompressor huff_decomp;
    
    if (!huff_decomp.decompress_file(huff_filename, lz77_recovered_filename)) {
        cout << "✗ Error en descompresión Huffman\n";
        return 1;
    }
    
    cout << "\n✓ Huffman descomprimido\n";
    cout << "✓ Guardado: " << lz77_recovered_filename << "\n\n";
    
    // ══════════════════════════════════════════════════════════════
    // PASO 3.5: INSPECCIONAR .lz77 RECUPERADO
    // ══════════════════════════════════════════════════════════════
    cout << "═══ INSPECCIÓN: .lz77 recuperado ═══\n";
    
    ifstream lz77_recovered_file(lz77_recovered_filename, ios::binary);
    
    // Leer header
    uint32_t num_tokens_recovered, original_size_recovered;
    lz77_recovered_file.read((char*)&num_tokens_recovered, 4);
    lz77_recovered_file.read((char*)&original_size_recovered, 4);
    
    cout << "Header LZ77 recuperado:\n";
    cout << "  num_tokens: " << num_tokens_recovered << "\n";
    cout << "  original_size: " << original_size_recovered << "\n";
    
    // Comparar headers
    bool headers_match = (num_tokens_lz77 == num_tokens_recovered) && 
                        (original_size_lz77 == original_size_recovered);
    
    if (headers_match) {
        cout << "  ✅ Headers coinciden\n\n";
    } else {
        cout << "  ❌ Headers NO coinciden\n";
        cout << "    Original: tokens=" << num_tokens_lz77 
             << " size=" << original_size_lz77 << "\n";
        cout << "    Recuperado: tokens=" << num_tokens_recovered 
             << " size=" << original_size_recovered << "\n\n";
        return 1;
    }
    
    // Leer primeros tokens
    cout << "Primeros 5 tokens del .lz77 recuperado:\n";
    for (int i = 0; i < 5 && i < num_tokens_recovered; i++) {
        Token t = Token::read_from(lz77_recovered_file);
        cout << "  Token " << i << ": ";
        if (t.type == LITERAL) {
            char c = (char)t.value;
            if (c >= 32 && c <= 126) {
                cout << "LITERAL '" << c << "'";
            } else {
                cout << "LITERAL (ASCII " << (int)t.value << ")";
            }
        } else {
            cout << "REF(len=" << t.value << ", dist=" << t.distance << ")";
        }
        cout << "\n";
    }
    
    lz77_recovered_file.close();
    cout << "\n";
    
    // ══════════════════════════════════════════════════════════════
    // PASO 4: DESCOMPRIMIR LZ77
    // ══════════════════════════════════════════════════════════════
    cout << "═══ PASO 4: Descompresión LZ77 ═══\n";
    
    // Leer archivo .lz77 recuperado completo
    ifstream lz77_in(lz77_recovered_filename, ios::binary);
    vector<uint8_t> lz77_recovered_data((istreambuf_iterator<char>(lz77_in)),
                                        istreambuf_iterator<char>());
    lz77_in.close();
    
    cout << "Archivo .lz77 recuperado: " << lz77_recovered_data.size() << " bytes\n";
    cout << "Tamaño esperado: 8 + (" << num_tokens_recovered << " * 5) = " 
         << (8 + num_tokens_recovered * 5) << " bytes\n";
    
    if (lz77_recovered_data.size() != (8 + num_tokens_recovered * 5)) {
        cout << "❌ PROBLEMA: Tamaño del archivo .lz77 incorrecto\n";
        cout << "   Diferencia: " << (int)lz77_recovered_data.size() - (int)(8 + num_tokens_recovered * 5) << " bytes\n\n";
        return 1;
    } else {
        cout << "✅ Tamaño correcto\n\n";
    }
    
    // Descomprimir
    LZ77 lz77_decomp;
    vector<uint8_t> recovered_bytes;
    
    cout << "Llamando a LZ77::decompress()...\n";
    
    if (!lz77_decomp.decompress(lz77_recovered_data.data(), 
                                lz77_recovered_data.size(), 
                                recovered_bytes)) {
        cout << "✗ Error en descompresión LZ77\n\n";
        
        cout << "🐛 DEBUG:\n";
        cout << "  Tamaño entrada: " << lz77_recovered_data.size() << "\n";
        cout << "  num_tokens en header: " << num_tokens_recovered << "\n";
        cout << "  original_size en header: " << original_size_recovered << "\n";
        
        return 1;
    }
    
    cout << "✓ Descomprimido: " << recovered_bytes.size() << " bytes\n\n";
    
    // Guardar archivo recuperado
    string recovered_filename = filename + ".recovered";
    ofstream recovered_file(recovered_filename, ios::binary);
    recovered_file.write((const char*)recovered_bytes.data(), recovered_bytes.size());
    recovered_file.close();
    
    cout << "✓ Guardado: " << recovered_filename << "\n\n";
    
    // ══════════════════════════════════════════════════════════════
    // PASO 5: VERIFICAR
    // ══════════════════════════════════════════════════════════════
    cout << "═══ VERIFICACIÓN FINAL ═══\n";
    
    bool identical = (original.size() == recovered_bytes.size()) &&
                    (memcmp(original.data(), recovered_bytes.data(), original.size()) == 0);
    
    if (identical) {
        cout << "✅ ¡ÉXITO TOTAL!\n";
        cout << "   Los archivos son IDÉNTICOS byte por byte\n\n";
        
        cout << "═══ ESTADÍSTICAS FINALES ═══\n";
        cout << "Original:    " << original.size() << " bytes (100%)\n";
        cout << "LZ77:        " << lz77_data.size() << " bytes (" << ratio_lz77 << "%)\n";
        cout << "Comprimido:  " << huff_size << " bytes (" 
             << (100.0 * huff_size / original.size()) << "%)\n";
        cout << "Recuperado:  " << recovered_bytes.size() << " bytes\n\n";
        
        if (huff_size < original.size()) {
            double ahorro = 100.0 - (100.0 * huff_size / original.size());
            cout << "✓ Ahorro total: " << ahorro << "%\n\n";
        } else {
            cout << "⚠ Archivo muy pequeño para comprimir eficientemente\n\n";
        }
        
        cout << "Archivos generados:\n";
        cout << "  " << lz77_filename << "\n";
        cout << "  " << huff_filename << " (ARCHIVO FINAL COMPRIMIDO)\n";
        cout << "  " << lz77_recovered_filename << "\n";
        cout << "  " << recovered_filename << " (ARCHIVO RECUPERADO)\n\n";
        
        return 0;
    } else {
        cout << "❌ ERROR: Los archivos son diferentes\n";
        cout << "   Tamaño original: " << original.size() << "\n";
        cout << "   Tamaño recuperado: " << recovered_bytes.size() << "\n\n";
        return 1;
    }
}