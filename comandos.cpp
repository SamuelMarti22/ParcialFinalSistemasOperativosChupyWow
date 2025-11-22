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
#include <omp.h>
#include <chrono>
#include "likeDeflate/deflate_interface.h"
#include "likeDeflate/folder_compressor.h"
#include "ChaCha20(encriptacion)/ChaCha20.h"
#include "ChaCha20(encriptacion)/sha256.h"
using namespace std;


// Función auxiliar para mostrar tiempo y bytes de una operación
static void mostrarResumenOperacion(const string& operacion, size_t bytes, double tiempoSegundos) {
    cout << endl;
    cout << "Operacion: " << operacion  << endl;
    cout << "Bytes procesados: " << bytes << " bytes" << endl;
    cout << "Tiempo: "<< tiempoSegundos << " s " << endl;
}


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

    if (necesitaEncriptacion && p.algoritmoEnc != "chacha20") {
        cerr << "\n Error: Solo el algoritmo 'chacha20' está soportado actualmente\n" << endl;
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
    cout << "  -c         Comprimir archivo/carpeta" << endl;
    cout << "  -d         Descomprimir archivo" << endl;
    cout << "  -e         Encriptar archivo" << endl;
    cout << "  -u         Desencriptar archivo" << endl;
    cout << "  -ce        Comprimir + Encriptar" << endl;
    cout << "  -ud        Desencriptar + Descomprimir\n" << endl;

    cout << "  -i <archivo>     Archivo/carpeta de entrada" << endl;
    cout << "  -o <archivo>     Archivo/carpeta de salida" << endl;
    cout << "  --comp-alg <x>   Algoritmo de compresión (deflate)" << endl;
    cout << "  --enc-alg <x>    Algoritmo de encriptación (chacha20)" << endl;
    cout << "  -k <clave>       Clave de encriptación\n" << endl;
    
    cout << "Variables de entorno:" << endl;
    cout << "  OMP_NUM_THREADS  Número de hilos para paralelización\n" << endl;
}

// Función para leer archivos usando syscalls POSIX
vector<uint8_t> leerArchivoConSyscalls(const string& rutaArchivo) {
    auto inicioLectura = chrono::high_resolution_clock::now();
    
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
    
    auto finLectura = chrono::high_resolution_clock::now();
    chrono::duration<double> duracion = finLectura - inicioLectura;
    
    cout << "Archivo leído exitosamente: " << rutaArchivo << " (" << totalLeido << " bytes)" << endl;
    mostrarResumenOperacion("Lectura de archivo", totalLeido, duracion.count());
    
    return buffer;
}

// Función para escribir archivos usando syscalls
void escribirArchivoConSyscalls(const string& rutaArchivo, const vector<uint8_t>& datos) {
    auto inicioEscritura = chrono::high_resolution_clock::now();
    
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
    
    auto finEscritura = chrono::high_resolution_clock::now();
    chrono::duration<double> duracion = finEscritura - inicioEscritura;
    
    cout << "Archivo escrito exitosamente: " << rutaArchivo << " (" << datos.size() << " bytes)" << endl;
    mostrarResumenOperacion("Escritura de archivo", datos.size(), duracion.count());
}

// Compresion de carpetas usando folder_compressor
// Explora la carpeta recursivamente y se obtienen todos los archivos
void comprimirCarpeta(const string& carpetaEntrada, const string& carpetaSalida, const string& algoritmo) {
    auto inicioCompresion = chrono::high_resolution_clock::now();
    
    cout << "Comprimiendo carpeta: " << carpetaEntrada << " -> " << carpetaSalida << endl;
    
    string salidaFinal = carpetaSalida;
    if (salidaFinal.find(".chupydir") == string::npos) {
        salidaFinal += ".chupydir";
    }
    
    FolderCompressor::compressFolder(carpetaEntrada, salidaFinal);
    
    // Obtener tamaño del archivo comprimido
    struct stat fileStat;
    size_t bytesComprimidos = 0;
    if (stat(salidaFinal.c_str(), &fileStat) == 0) {
        bytesComprimidos = fileStat.st_size;
    }
    
    auto finCompresion = chrono::high_resolution_clock::now();
    chrono::duration<double> duracion = finCompresion - inicioCompresion;
    
    cout << "Compresión de carpeta completada." << endl;
    mostrarResumenOperacion("Compresión de carpeta", bytesComprimidos, duracion.count());
}

void descomprimirCarpeta(const string& archivoEntrada, const string& carpetaSalida, const string& algoritmo) {
    auto inicioDescompresion = chrono::high_resolution_clock::now();
    
    cout << "Descomprimiendo archivo: " << archivoEntrada << " -> " << carpetaSalida << endl;
    
    // Obtener tamaño del archivo antes de descomprimir
    struct stat fileStat;
    size_t bytesComprimidos = 0;
    if (stat(archivoEntrada.c_str(), &fileStat) == 0) {
        bytesComprimidos = fileStat.st_size;
    }
    
    FolderCompressor::decompressFolder(archivoEntrada, carpetaSalida);
    
    auto finDescompresion = chrono::high_resolution_clock::now();
    chrono::duration<double> duracion = finDescompresion - inicioDescompresion;
    
    cout << "Descompresión de carpeta completada." << endl;
    mostrarResumenOperacion("Descompresión de carpeta", bytesComprimidos, duracion.count());
}


// Encriptación y desencriptación usando ChaCha20
void encriptarArchivo(const string& archivoEntrada, const string& archivoSalida, const string& password) {
    auto inicioEncriptacion = chrono::high_resolution_clock::now();
    
    cout << "Encriptando archivo: " << archivoEntrada << " -> " << archivoSalida << endl;
    cout << "Algoritmo: ChaCha20" << endl;
    
    // Obtener tamaño del archivo de entrada
    struct stat fileStat;
    size_t bytesEncriptados = 0;
    if (stat(archivoEntrada.c_str(), &fileStat) == 0) {
        bytesEncriptados = fileStat.st_size;
    }
    
    uint8_t key[CHACHA20_KEY_SIZE];
    SHA256::hash(password, key);
    
    chacha20_encrypt_file(archivoEntrada, archivoSalida, key);
    
    memset(key, 0, CHACHA20_KEY_SIZE);
    
    auto finEncriptacion = chrono::high_resolution_clock::now();
    chrono::duration<double> duracion = finEncriptacion - inicioEncriptacion;
    
    cout << "Encriptación completada." << endl;
    mostrarResumenOperacion("Encriptación (ChaCha20)", bytesEncriptados, duracion.count());
}

void desencriptarArchivo(const string& archivoEntrada, const string& archivoSalida, const string& password) {
    auto inicioDesencriptacion = chrono::high_resolution_clock::now();
    
    cout << "Desencriptando archivo: " << archivoEntrada << " -> " << archivoSalida << endl;
    cout << "Algoritmo: ChaCha20" << endl;
    
    // Obtener tamaño del archivo encriptado
    struct stat fileStat;
    size_t bytesDesencriptados = 0;
    if (stat(archivoEntrada.c_str(), &fileStat) == 0) {
        bytesDesencriptados = fileStat.st_size;
    }
    
    uint8_t key[CHACHA20_KEY_SIZE];
    SHA256::hash(password, key);
    
    chacha20_decrypt_file(archivoEntrada, archivoSalida, key);
    
    memset(key, 0, CHACHA20_KEY_SIZE);
    
    auto finDesencriptacion = chrono::high_resolution_clock::now();
    chrono::duration<double> duracion = finDesencriptacion - inicioDesencriptacion;
    
    cout << "Desencriptación completada." << endl;
    mostrarResumenOperacion("Desencriptación (ChaCha20)", bytesDesencriptados, duracion.count());
}

// Detectar si el archivo es de carpeta comprimida (.chupydir)
static bool esArchivoCarpetaComprimida(const string& archivo) {
    // Detectar archivos .chupydir
    return archivo.find(".chupydir") != string::npos;
}

void ejecutarOperacion(const Parametros& params) {
    try {
        cout << "Entrada: " << params.entrada << " -> Salida: " << params.salida << endl;

        // Detectar tipo usando syscall 
        struct stat entryStat;
        if (stat(params.entrada.c_str(), &entryStat) == -1) {
            throw runtime_error("Error: No se pudo acceder a la entrada: " + params.entrada);
        }

        bool esDirectorio = S_ISDIR(entryStat.st_mode);
        bool esArchivo = S_ISREG(entryStat.st_mode);
        bool esCarpetaComprimida = esArchivoCarpetaComprimida(params.entrada);

        // Operaciones combinadas 
        if (params.comprimirYEncriptar) {
            cout << "Detectado: Comprimir + Encriptar" << endl;
            
            string archivoTemp = params.salida + ".temp.chupy";
            
            if (esDirectorio) {
                comprimirCarpeta(params.entrada, archivoTemp, params.algoritmoComp);
            } else {
                comprimirConDeflate(params.entrada, archivoTemp);
            }
            
            encriptarArchivo(archivoTemp, params.salida, params.clave);
            unlink(archivoTemp.c_str());
            
        } else if (params.desencriptarYDescomprimir) {
            cout << "Detectado: Desencriptar + Descomprimir" << endl;
            
            string archivoTemp = params.entrada + ".temp";
            
            desencriptarArchivo(params.entrada, archivoTemp, params.clave);
            
            // Detectar si es carpeta o archivo por extensión del temp
            if (esArchivoCarpetaComprimida(archivoTemp)) {
                descomprimirCarpeta(archivoTemp, params.salida, params.algoritmoComp);
            } else {
                descomprimirConDeflate(archivoTemp, params.salida);
            }
            
            unlink(archivoTemp.c_str());
            
        // Solo encriptación/desencriptación
        } else if (params.encriptar) {
            cout << "Detectado: Solo Encriptar" << endl;
            encriptarArchivo(params.entrada, params.salida, params.clave);
            
        } else if (params.desencriptar) {
            cout << "Detectado: Solo Desencriptar" << endl;
            desencriptarArchivo(params.entrada, params.salida, params.clave);
            
        // Solo compresion/descompresión
        } else if (params.comprimir) {
            if (esDirectorio) {
                cout << "Detectado: carpeta" << endl;
                comprimirCarpeta(params.entrada, params.salida, params.algoritmoComp);
            } else if (esArchivo) {
                cout << "Detectado: archivo" << endl;
                comprimirConDeflate(params.entrada, params.salida);
            } else {
                throw runtime_error("Error: Tipo de entrada no soportado");
            }
            
        } else if (params.descomprimir) {
            if (esCarpetaComprimida) {
                cout << "Detectado: archivo de carpeta comprimida (.chupydir)" << endl;
                descomprimirCarpeta(params.entrada, params.salida, params.algoritmoComp);
            } else if (esArchivo) {
                cout << "Detectado: archivo comprimido individual" << endl;
                descomprimirConDeflate(params.entrada, params.salida);
            } else {
                throw runtime_error("Error: Tipo de entrada no soportado para descompresión");
            }
        }

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        exit(1);
    }
}