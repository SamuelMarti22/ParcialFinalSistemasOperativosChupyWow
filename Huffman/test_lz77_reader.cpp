// test_lz77_reader.cpp
#include <iostream>
#include <fstream>
#include "lz77_reader.h"
#include "common.h"

using namespace std;

// Función auxiliar para crear un archivo .lz77 de prueba
void crear_archivo_prueba(const string& filename) {
    cout << "=== CREANDO ARCHIVO DE PRUEBA: " << filename << " ===\n\n";
    
    ofstream file(filename, ios::binary);
    
    if (!file.is_open()) {
        cerr << "Error: No se pudo crear " << filename << endl;
        return;
    }
    
    // ═══════════════════════════════════
    //  PASO 1: ESCRIBIR HEADER (8 bytes)
    // ═══════════════════════════════════
    
    uint32_t num_tokens = 6;      // Vamos a escribir 6 tokens
    uint32_t original_size = 9;   // Texto original: "hola hola" = 9 bytes
    
    file.write((const char*)&num_tokens, sizeof(num_tokens));
    file.write((const char*)&original_size, sizeof(original_size));
    
    cout << "Header escrito:\n";
    cout << "  - num_tokens: " << num_tokens << "\n";
    cout << "  - original_size: " << original_size << " bytes\n\n";
    
    // ═══════════════════════════════════
    //  PASO 2: ESCRIBIR TOKENS
    // ═══════════════════════════════════
    
    cout << "Escribiendo tokens:\n";
    
    // Token 0: LITERAL 'h' (ASCII 104)
    Token t0 = {0, 104, 0};
    t0.write_to(file);
    cout << "  Token 0: LITERAL 'h' (ASCII 104)\n";
    
    // Token 1: LITERAL 'o' (ASCII 111)
    Token t1 = {0, 111, 0};
    t1.write_to(file);
    cout << "  Token 1: LITERAL 'o' (ASCII 111)\n";
    
    // Token 2: LITERAL 'l' (ASCII 108)
    Token t2 = {0, 108, 0};
    t2.write_to(file);
    cout << "  Token 2: LITERAL 'l' (ASCII 108)\n";
    
    // Token 3: LITERAL 'a' (ASCII 97)
    Token t3 = {0, 97, 0};
    t3.write_to(file);
    cout << "  Token 3: LITERAL 'a' (ASCII 97)\n";
    
    // Token 4: LITERAL ' ' (ASCII 32)
    Token t4 = {0, 32, 0};
    t4.write_to(file);
    cout << "  Token 4: LITERAL ' ' (espacio, ASCII 32)\n";
    
    // Token 5: REFERENCE (copiar 4 caracteres desde 5 posiciones atrás)
    Token t5 = {1, 4, 5};
    t5.write_to(file);
    cout << "  Token 5: REFERENCE (len=4, dist=5)\n";
    cout << "           → Copia 'hola' desde 5 posiciones atrás\n";
    
    file.close();
    
    // Calcular tamaño del archivo
    ifstream check(filename, ios::binary | ios::ate);
    size_t file_size = check.tellg();
    check.close();
    
    cout << "\n✓ Archivo creado exitosamente\n";
    cout << "  Tamaño del archivo: " << file_size << " bytes\n";
    cout << "  Esperado: 8 (header) + 30 (6 tokens × 5) = 38 bytes\n\n";
}

// Función para mostrar el contenido hexadecimal del archivo
void mostrar_hex(const string& filename) {
    cout << "=== CONTENIDO HEXADECIMAL DE: " << filename << " ===\n\n";
    
    ifstream file(filename, ios::binary);
    
    if (!file.is_open()) {
        cerr << "Error: No se pudo abrir " << filename << endl;
        return;
    }
    
    cout << "Offset  Bytes                              Descripción\n";
    cout << "------  ---------------------------------  ---------------------\n";
    
    // Leer header
    uint8_t header[8];
    file.read((char*)header, 8);
    
    cout << "0x0000  ";
    for (int i = 0; i < 8; i++) {
        printf("%02X ", header[i]);
    }
    cout << "                 HEADER\n";
    
    cout << "        ";
    for (int i = 0; i < 4; i++) {
        printf("%02X ", header[i]);
    }
    cout << "                           num_tokens\n";
    
    cout << "        ";
    for (int i = 4; i < 8; i++) {
        printf("%02X ", header[i]);
    }
    cout << "                           original_size\n\n";
    
    // Leer tokens
    int offset = 8;
    int token_num = 0;
    
    while (file) {
        uint8_t token_bytes[5];
        file.read((char*)token_bytes, 5);
        
        if (!file) break;
        
        printf("0x%04X  ", offset);
        for (int i = 0; i < 5; i++) {
            printf("%02X ", token_bytes[i]);
        }
        
        // Interpretar el token
        if (token_bytes[0] == 0) {
            // LITERAL
            char c = token_bytes[1];
            if (c >= 32 && c <= 126) {
                printf("                 Token %d: LITERAL '%c'\n", token_num, c);
            } else {
                printf("                 Token %d: LITERAL (ASCII %d)\n", token_num, (int)c);
            }
        } else {
            // REFERENCE
            uint16_t value = token_bytes[1] | (token_bytes[2] << 8);
            uint16_t distance = token_bytes[3] | (token_bytes[4] << 8);
            printf("                 Token %d: REF (len=%d, dist=%d)\n", 
                   token_num, value, distance);
        }
        
        offset += 5;
        token_num++;
    }
    
    file.close();
    cout << "\n";
}

// Función principal de prueba
void probar_lectura(const string& filename) {
    cout << "=== PROBANDO LZ77Reader::read_file() ===\n\n";
    
    // Leer el archivo
    vector<Token> tokens = LZ77Reader::read_file(filename);
    
    if (tokens.empty()) {
        cout << "❌ No se pudieron leer tokens\n";
        return;
    }
    
    cout << "\n✓ Tokens leídos correctamente\n";
    cout << "  Total: " << tokens.size() << " tokens\n\n";
    
    // Mostrar cada token
    cout << "Detalle de tokens:\n";
    cout << "─────────────────────────────────────────────────────\n";
    
    for (size_t i = 0; i < tokens.size(); i++) {
        cout << "Token " << i << ": ";
        
        if (tokens[i].type == 0) {
            // LITERAL
            char c = (char)tokens[i].value;
            cout << "LITERAL";
            
            if (c >= 32 && c <= 126) {
                cout << " '" << c << "'";
            }
            
            cout << " (ASCII " << tokens[i].value << ")\n";
            cout << "         type=" << (int)tokens[i].type 
                 << ", value=" << tokens[i].value 
                 << ", distance=" << tokens[i].distance << "\n";
            
        } else {
            // REFERENCE
            cout << "REFERENCE (len=" << tokens[i].value 
                 << ", dist=" << tokens[i].distance << ")\n";
            cout << "         type=" << (int)tokens[i].type 
                 << ", value=" << tokens[i].value 
                 << ", distance=" << tokens[i].distance << "\n";
            cout << "         → Copiar " << tokens[i].value 
                 << " caracteres desde " << tokens[i].distance 
                 << " posiciones atrás\n";
        }
        
        cout << "\n";
    }
}

// Función para probar solo lectura de header
void probar_header(const string& filename) {
    cout << "=== PROBANDO LZ77Reader::read_header() ===\n\n";
    
    uint32_t num_tokens, original_size;
    LZ77Reader::read_header(filename, num_tokens, original_size);
    
    cout << "Información del header:\n";
    cout << "  - Número de tokens: " << num_tokens << "\n";
    cout << "  - Tamaño original: " << original_size << " bytes\n\n";
}

// Función para reconstruir el texto original
void reconstruir_texto(const string& filename) {
    cout << "=== RECONSTRUYENDO TEXTO ORIGINAL ===\n\n";
    
    vector<Token> tokens = LZ77Reader::read_file(filename);
    
    if (tokens.empty()) {
        cout << "❌ No se pudieron leer tokens\n";
        return;
    }
    
    // Buffer para reconstruir
    string resultado = "";
    
    cout << "Proceso de reconstrucción:\n";
    cout << "─────────────────────────────────────────────────────\n";
    
    for (size_t i = 0; i < tokens.size(); i++) {
        cout << "Token " << i << ": ";
        
        if (tokens[i].type == 0) {
            // LITERAL - agregar el carácter
            char c = (char)tokens[i].value;
            resultado += c;
            cout << "Agregar '" << c << "' → resultado = \"" << resultado << "\"\n";
            
        } else {
            // REFERENCE - copiar desde atrás
            int pos_inicio = resultado.size() - tokens[i].distance;
            
            cout << "Copiar " << tokens[i].value << " desde posición " << pos_inicio << "\n";
            cout << "       Antes: \"" << resultado << "\"\n";
            
            for (int j = 0; j < tokens[i].value; j++) {
                resultado += resultado[pos_inicio + j];
            }
            
            cout << "       Después: \"" << resultado << "\"\n";
        }
    }
    
    cout << "\n✓ Reconstrucción completa\n";
    cout << "  Texto original: \"" << resultado << "\"\n";
    cout << "  Longitud: " << resultado.size() << " bytes\n\n";
}

int main() {
    const string filename = "test.lz77";
    
    cout << "╔════════════════════════════════════════════════════════╗\n";
    cout << "║      PRUEBA COMPLETA DE LZ77 READER                   ║\n";
    cout << "╚════════════════════════════════════════════════════════╝\n\n";
    
    // Paso 1: Crear archivo de prueba
    crear_archivo_prueba(filename);
    
    // Paso 2: Mostrar contenido hexadecimal
    mostrar_hex(filename);
    
    // Paso 3: Probar lectura de header
    probar_header(filename);
    
    // Paso 4: Probar lectura completa
    probar_lectura(filename);
    
    // Paso 5: Reconstruir texto original
    reconstruir_texto(filename);
    
    cout << "╔════════════════════════════════════════════════════════╗\n";
    cout << "║      PRUEBA COMPLETADA                                 ║\n";
    cout << "╚════════════════════════════════════════════════════════╝\n";
    
    return 0;
}