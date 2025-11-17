#include "comandos.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdexcept>
#include <cstring>
#include <iomanip>
#include <cstdint>
#include <dirent.h>
#include <errno.h>
using namespace std;

// Estructura para información de archivos en el contenedor usado para carpetas
struct ArchivoInfo {
    string rutaRelativa;
    vector<uint8_t> contenido;
};


// Método que convierte los argumentos de línea de comandos en una estructura Parametros
// Parsea los argumentos sin validar
static Parametros parsearArgumentos(int argc, char* argv[]) {
    Parametros p;

    if (argc == 1) {
        mostrarAyuda();
        exit(0);
    }

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            mostrarAyuda();
            exit(0);
        }
        else if (arg == "-c") {
            p.comprimir = true;
        }
        else if (arg == "-d") {
            p.descomprimir = true;
        }
        else if (arg == "-e") {
            p.encriptar = true;
        }
        else if (arg == "-u") {
            p.desencriptar = true;
        }
        else if (arg == "-ce") {
            p.comprimirYEncriptar = true;
        }
        else if (arg == "-ud") {
            p.desencriptarYDescomprimir = true;
        }
        else if (arg == "--comp-alg") {
            if (i + 1 < argc) {
                p.algoritmoComp = argv[++i];
            } else {
                cerr << "\n Error: --comp-alg requiere un algoritmo" << endl;
                exit(1);
            }
        }
        else if (arg == "--enc-alg") {
            if (i + 1 < argc) {
                p.algoritmoEnc = argv[++i];
            } else {
                cerr << "\nError: --enc-alg requiere un algoritmo" << endl;
                exit(1);
            }
        }
        else if (arg == "-i") {
            if (i + 1 < argc) {
                p.entrada = argv[++i];
            } else {
                cerr << "\n Error: -i requiere de una ruta" << endl;
                exit(1);
            }
        }
        else if (arg == "-o") {
            if (i + 1 < argc) {
                p.salida = argv[++i];
            } else {
                cerr << "\n Error: -o requiere una ruta" << endl;
                exit(1);
            }
        }
        else if (arg == "-k") {
            if (i + 1 < argc) {
                p.clave = argv[++i];
            } else {
                cerr << "\n Error: -k requiere una clave" << endl;
                exit(1);
            }
        }
        else {
            cerr << "\nError: Comando desconocido: " << arg << endl;
            cerr << "Puedes usar -h o --help para ver más ayuda\n" << endl;
            exit(1);
        }
    }

    return p;
}

// Funcion que verifica que la combinación de parámetros sea lógica y completa
static void validarLogicaParametros(const Parametros& p) {
    bool hayOperacion = p.comprimir || p.descomprimir || p.encriptar || 
                        p.desencriptar || p.comprimirYEncriptar || 
                        p.desencriptarYDescomprimir;
    
    if (!hayOperacion) {
        cerr << "\nError: Debes especificar una operación" << endl;
        cerr << "Usa -h para más ayuda\n" << endl;
        exit(1);
    }

    if (p.comprimir && p.descomprimir) {
        cerr << "\nError: No puedes usar -c y -d juntos\n" << endl;
        exit(1);
    }

    if (p.encriptar && p.desencriptar) {
        cerr << "\nError: No puedes usar -e y -u juntos\n" << endl;
        exit(1);
    }

    if ((p.comprimir || p.descomprimir) && p.comprimirYEncriptar) {
        cerr << "\nError: No uses -c o -d junto con -ce\n" << endl;
        exit(1);
    }

    if ((p.encriptar || p.desencriptar) && p.desencriptarYDescomprimir) {
        cerr << "\nError: No uses -e o -u junto con -ud\n" << endl;
        exit(1);
    }

    if (p.comprimirYEncriptar && p.desencriptarYDescomprimir) {
        cerr << "\nError: No puedes usar -ce y -ud juntos\n" << endl;
        exit(1);
    }

    if (p.entrada.empty()) {
        cerr << "\nError: Debes especificar un archivo de entrada con -i\n" << endl;
        exit(1);
    }

    if (p.salida.empty()) {
        cerr << "\nError: Debes especificar archivo de salida con -o\n" << endl;
        exit(1);
    }

    bool necesitaCompresion = p.comprimir || p.descomprimir || p.comprimirYEncriptar || 
                              p.desencriptarYDescomprimir;
    
    if (necesitaCompresion && p.algoritmoComp.empty()) {
        cerr << "\nError: Debes especificar algun algoritmo con --comp-alg\n" << endl;
        exit(1);
    }

    bool necesitaEncriptacion = p.encriptar || p.desencriptar || p.comprimirYEncriptar || 
                                p.desencriptarYDescomprimir;
    
    if (necesitaEncriptacion && p.algoritmoEnc.empty()) {
        cerr << "\n Error: Debes especificar algoritmo con --enc-alg\n" << endl;
        exit(1);
    }

    if (necesitaEncriptacion && p.clave.empty()) {
        cerr << "\n Error: Debes especificar una clave con -k\n" << endl;
        exit(1);
    }
}

Parametros leerYValidarComandos(int argc, char* argv[]) {
    Parametros params = parsearArgumentos(argc, argv);
    validarLogicaParametros(params);
    return params;
}

void mostrarAyuda() {
    cout << "Uso: ./xxxx [opciones]\n" << endl;
    cout << "Comandos:" << endl;
    cout << "  -c         Comprimir archivo" << endl;
    cout << "  -d         Descomprimir archivo" << endl;
    cout << "  -e         Encriptar archivo" << endl;
    cout << "  -u         Desencriptar archivo" << endl;
    cout << "  -ce        Comprimir + Encriptar" << endl;
    cout << "  -ud        Desencriptar + Descomprimir\n" << endl;

    cout << "  -i <archivo>     Archivo de entrada" << endl;
    cout << "  -o <archivo>     Archivo de salida" << endl;
    cout << "  --comp-alg <x>   Algoritmo de compresión (ej: huffman)" << endl;
    cout << "  --enc-alg <x>    Algoritmo de encriptación (ej: chacha20)" << endl;
    cout << "  -k <clave>       Clave de encriptación\n" << endl;
}

// Función para leer archivos usando syscalls POSIX
vector<uint8_t> leerArchivoConSyscalls(const string& rutaArchivo) {
    // Abrir archivo para lectura
    int fd = open(rutaArchivo.c_str(), O_RDONLY);
    if (fd == -1) {
        throw runtime_error(" No se pudo abrir el archivo para lectura: " + rutaArchivo + " (" + strerror(errno) + ")");
    }
    struct stat fileStat;
    if (fstat(fd, &fileStat) == -1) {
        close(fd);
        throw runtime_error("No se pudo obtener información del archivo: " + rutaArchivo);
    }

    size_t fileSize = fileStat.st_size;
    vector<uint8_t> buffer(fileSize);
    ssize_t bytesLeidos = 0;
    ssize_t totalLeido = 0;

    while (totalLeido < static_cast<ssize_t>(fileSize)) {
        bytesLeidos = read(fd, buffer.data() + totalLeido, fileSize - totalLeido);
        if (bytesLeidos == -1) {
            close(fd);
            throw runtime_error("Error: Fallo al leer el archivo: " + rutaArchivo + " (" + strerror(errno) + ")");
        }
        if (bytesLeidos == 0) {
            break; 
        }
        totalLeido += bytesLeidos;
    }
    close(fd);
    buffer.resize(totalLeido);
    cout << "Archivo leído exitosamente: " << rutaArchivo << " (" << totalLeido << " bytes)" << endl;
    return buffer;
}

// Función para escribir archivos usando syscalls
void escribirArchivoConSyscalls(const string& rutaArchivo, const vector<uint8_t>& datos) {
    int fd = open(rutaArchivo.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        throw runtime_error("No se pudo abrir el archivo para escritura: " + rutaArchivo + " (" + strerror(errno) + ")");
    }
    ssize_t bytesEscritos = 0;
    ssize_t totalEscrito = 0;
    size_t dataSize = datos.size();

    while (totalEscrito < static_cast<ssize_t>(dataSize)) {
        bytesEscritos = write(fd, datos.data() + totalEscrito, dataSize - totalEscrito);
        if (bytesEscritos == -1) {
            close(fd);
            throw runtime_error("Fallo al escribir el archivo: " + rutaArchivo + " (" + strerror(errno) + ")");
        }
        totalEscrito += bytesEscritos;
    }

    close(fd);
    cout << "Archivo escrito exitosamente: " << rutaArchivo << " (" << datos.size() << " bytes)" << endl;
}

