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
#include "likeDeflate/deflate_interface.h"
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

// Explora la carpeta recursivamente y se obtienen todos los archivos
vector<ArchivoInfo> explorarCarpetaRecursivo(const string& carpetaBase, const string& carpetaActual = "") {
    vector<ArchivoInfo> archivos;
    string rutaCompleta = carpetaBase;
    if (!carpetaActual.empty()) {
        rutaCompleta += "/" + carpetaActual;
    }
    DIR* dir = opendir(rutaCompleta.c_str());
    if (dir == nullptr) {
        throw runtime_error("Error: No se pudo abrir la carpeta: " + rutaCompleta);
    }
    struct dirent* entry;
    vector<pair<string, string>> archivosAProcesar; // rutaCompleta, rutaRelativa
    vector<string> subcarpetas;
    while ((entry = readdir(dir)) != nullptr) {
        string nombre = entry->d_name;
        if (nombre == "." || nombre == "..") continue;
        string rutaArchivoCompleta = rutaCompleta + "/" + nombre;
        string rutaRelativaArchivo = carpetaActual.empty() ? nombre : carpetaActual + "/" + nombre;
        struct stat entryStat;
        if (stat(rutaArchivoCompleta.c_str(), &entryStat) == -1) {
            cerr << "  Advertencia: No se pudo acceder a " << rutaArchivoCompleta << endl;
            continue;
        }
        if (S_ISREG(entryStat.st_mode)) {
            archivosAProcesar.push_back({rutaArchivoCompleta, rutaRelativaArchivo});
        } else if (S_ISDIR(entryStat.st_mode)) {
            subcarpetas.push_back(rutaRelativaArchivo);
        }
    }
    closedir(dir);

    // Paralelizar la lectura de archivos
    #pragma omp parallel for default(none) shared(archivos, archivosAProcesar)
    for (size_t i = 0; i < archivosAProcesar.size(); ++i) {
        ArchivoInfo info;
        info.rutaRelativa = archivosAProcesar[i].second;
        info.contenido = leerArchivoConSyscalls(archivosAProcesar[i].first);
        #pragma omp critical
        {
            archivos.push_back(info);
        }
    }

    // Procesar subcarpetas (recursivo, secuencial)
    for (const auto& sub : subcarpetas) {
        auto subArchivos = explorarCarpetaRecursivo(carpetaBase, sub);
        archivos.insert(archivos.end(), subArchivos.begin(), subArchivos.end());
    }
    return archivos;
}

// Función para crear el contenedor 
vector<uint8_t> crearContenedor(const vector<ArchivoInfo>& archivos) {
    vector<uint8_t> contenedor;
    
    // Header: número de archivos
    uint32_t numArchivos = static_cast<uint32_t>(archivos.size());
    contenedor.push_back((numArchivos >> 24) & 0xFF);
    contenedor.push_back((numArchivos >> 16) & 0xFF);
    contenedor.push_back((numArchivos >> 8) & 0xFF);
    contenedor.push_back(numArchivos & 0xFF);
    
    // Por cada archivo tiene: [tamaño_nombre][nombre][tamaño_contenido][contenido]
    for (const auto& archivo : archivos) {
        // Tamaño del nombre 
        uint32_t tamanoNombre = static_cast<uint32_t>(archivo.rutaRelativa.size());
        contenedor.push_back((tamanoNombre >> 24) & 0xFF);
        contenedor.push_back((tamanoNombre >> 16) & 0xFF);
        contenedor.push_back((tamanoNombre >> 8) & 0xFF);
        contenedor.push_back(tamanoNombre & 0xFF);
        
        // Nombre del archivo
        for (char c : archivo.rutaRelativa) {
            contenedor.push_back(static_cast<uint8_t>(c));
        }
        // Tamaño del contenido 
        uint32_t tamanoContenido = static_cast<uint32_t>(archivo.contenido.size());
        contenedor.push_back((tamanoContenido >> 24) & 0xFF);
        contenedor.push_back((tamanoContenido >> 16) & 0xFF);
        contenedor.push_back((tamanoContenido >> 8) & 0xFF);
        contenedor.push_back(tamanoContenido & 0xFF);
        
        // Contenido del archivo
        contenedor.insert(contenedor.end(), archivo.contenido.begin(), archivo.contenido.end());
    }
    
    return contenedor;
}

// Función para parsear el contenedor para el momento de descomprimir
vector<ArchivoInfo> parsearContenedor(const vector<uint8_t>& contenedor) {
    vector<ArchivoInfo> archivos;
    if (contenedor.size() < 4) {
        throw runtime_error("Error: Contenedor demasiado pequeño");
    }
    uint32_t numArchivos = (static_cast<uint32_t>(contenedor[0]) << 24) |(static_cast<uint32_t>(contenedor[1]) << 16) |(static_cast<uint32_t>(contenedor[2]) << 8) | static_cast<uint32_t>(contenedor[3]);
    size_t offset = 4;
   
    for (uint32_t i = 0; i < numArchivos; ++i) {
        if (offset + 4 > contenedor.size()) {
            throw runtime_error("Error: Contenedor corrupto al leer archivo " + to_string(i));
        }
        
        // Leer tamaño del nombre
        uint32_t tamanoNombre = (static_cast<uint32_t>(contenedor[offset]) << 24) | (static_cast<uint32_t>(contenedor[offset + 1]) << 16) | (static_cast<uint32_t>(contenedor[offset + 2]) << 8) | static_cast<uint32_t>(contenedor[offset + 3]);
        offset += 4;
        
        if (offset + tamanoNombre > contenedor.size()) {
            throw runtime_error("Error: Contenedor corrupto al leer nombre del archivo " + to_string(i));
        }
        
        // Leer nombre
        string nombre(contenedor.begin() + offset, contenedor.begin() + offset + tamanoNombre);
        offset += tamanoNombre;
        
        if (offset + 4 > contenedor.size()) {
            throw runtime_error("Error: Contenedor corrupto al leer tamaño del contenido " + to_string(i));
        }
        
        // Leer tamaño del contenido
        uint32_t tamanoContenido = (static_cast<uint32_t>(contenedor[offset]) << 24) | (static_cast<uint32_t>(contenedor[offset + 1]) << 16) | (static_cast<uint32_t>(contenedor[offset + 2]) << 8) | static_cast<uint32_t>(contenedor[offset + 3]);
        offset += 4;
        
        if (offset + tamanoContenido > contenedor.size()) {
            throw runtime_error("Error: Contenedor corrupto al leer contenido del archivo " + to_string(i));
        }
        
        // Leer contenido
        vector<uint8_t> contenido(contenedor.begin() + offset, contenedor.begin() + offset + tamanoContenido);
        offset += tamanoContenido;
        
        // Agregar archivo
        ArchivoInfo info;
        info.rutaRelativa = nombre;
        info.contenido = contenido;
        archivos.push_back(info);
    }
    return archivos;
}

// Función para crear estructura de carpetas
void crearEstructuraCarpetas(const string& rutaBase, const string& rutaArchivo) {
    size_t pos = rutaArchivo.find_last_of('/');
    if (pos != string::npos) {
        string carpetaParent = rutaBase + "/" + rutaArchivo.substr(0, pos);
        string carpetaAcumulada = rutaBase;   // Crear carpetas recursivamente
        string subcarpeta = rutaArchivo.substr(0, pos);
        size_t inicio = 0;
        while (inicio < subcarpeta.size()) {
            size_t siguiente = subcarpeta.find('/', inicio);
            if (siguiente == string::npos) siguiente = subcarpeta.size();
            
            carpetaAcumulada += "/" + subcarpeta.substr(inicio, siguiente - inicio);
            
            if (mkdir(carpetaAcumulada.c_str(), 0755) == -1 && errno != EEXIST) {
                cerr << "  Advertencia: No se pudo crear carpeta: " << carpetaAcumulada << endl;
            }
            
            inicio = siguiente + 1;
        }
    }
}

void comprimirCarpeta(const string& carpetaEntrada, const string& carpetaSalida, const string& algoritmo) {
    string algoritmoUsado;
    if (algoritmo.empty()) {
        algoritmoUsado = "deflate";
    } else {
        algoritmoUsado = "deflate"; 
    }
    cout << "Comprimiendo carpeta: " << carpetaEntrada << " -> " << carpetaSalida << endl;
    cout << "Algoritmo: " << algoritmoUsado << " (modo contenedor)" << endl;
    // Explorar carpeta y obtener todos los archivos
    vector<ArchivoInfo> archivos = explorarCarpetaRecursivo(carpetaEntrada);
    
    if (archivos.empty()) {
        cout << "Advertencia: No se encontraron archivos para comprimir." << endl;
        return;
    }
    cout << "Total de archivos encontrados: " << archivos.size() << endl;

    // Crear contenedor con todos los archivos
    vector<uint8_t> contenedor = crearContenedor(archivos);
    cout << "Contenedor creado (" << contenedor.size() << " bytes)" << endl;
    
    // contenedor temporal y comprimirlo con deflate -> llamar a la interfaz
    string contenedorTemp = carpetaSalida + "_temp.bin";
    escribirArchivoConSyscalls(contenedorTemp, contenedor);
    
    // Comprimir usando la interfaz de deflate (agregará .chupy automáticamente)
    comprimirConDeflate(contenedorTemp, carpetaSalida);
    unlink(contenedorTemp.c_str());
    
    cout << "Compresión de carpeta completada." << endl;
}

void descomprimirCarpeta(const string& archivoEntrada, const string& carpetaSalida, const string& algoritmo) {
    // Usar deflate por defecto, independientemente del algoritmo especificado
    string algoritmoUsado;
    if (algoritmo.empty()) {
        algoritmoUsado = "deflate";
    } else {
        algoritmoUsado = "deflate"; // Siempre usar deflate por ahora
    }
    cout << "Descomprimiendo archivo: " << archivoEntrada << " -> " << carpetaSalida << endl;
    cout << "Algoritmo usado: " << algoritmoUsado << endl;
    
    string contenedorTemp = carpetaSalida + "_temp.bin";
    
    try {
        descomprimirConDeflate(archivoEntrada, contenedorTemp);
        
        vector<uint8_t> contenedor = leerArchivoConSyscalls(contenedorTemp);
        vector<ArchivoInfo> archivos = parsearContenedor(contenedor);
        cout << "Archivos encontrados en el contenedor: " << archivos.size() << endl;
        
        if (mkdir(carpetaSalida.c_str(), 0755) == -1 && errno != EEXIST) {
            throw runtime_error("Error: No se pudo crear la carpeta de salida: " + carpetaSalida);
        }
        
        #pragma omp parallel for default(none) shared(archivos, carpetaSalida)
        for (size_t i = 0; i < archivos.size(); ++i) {
            string rutaCompleta = carpetaSalida + "/" + archivos[i].rutaRelativa;
            crearEstructuraCarpetas(carpetaSalida, archivos[i].rutaRelativa);
            escribirArchivoConSyscalls(rutaCompleta, archivos[i].contenido);
        }
        
        cout << "Descompresión de carpeta completada." << endl;
        
    } catch (...) {
        unlink(contenedorTemp.c_str());
        throw; 
    }
    
    unlink(contenedorTemp.c_str());
}

void ejecutarOperacion(const Parametros& params) {
    try {
        cout << "Entrada: " << params.entrada << " -> Salida: " << params.salida << endl;

        // Detectar tipo usando syscall 
        struct stat entryStat;
        if (stat(params.entrada.c_str(), &entryStat) == -1) {
            throw runtime_error("Error: No se pudo acceder a la entrada: " + params.entrada);
        }

        // Mira si es carpeta comprimida para descomprimir
        bool esCarpetaComprimida = S_ISREG(entryStat.st_mode) &&  params.entrada.find(".chupy") != string::npos && (params.descomprimir || params.desencriptarYDescomprimir) && params.salida.find(".") == string::npos;

        if (esCarpetaComprimida) {
            cout << "Detectado: archivo de carpeta comprimida" << endl;
            descomprimirCarpeta(params.entrada, params.salida, params.algoritmoComp);
        } else if (S_ISREG(entryStat.st_mode)) {
            cout << "Detectado: archivo" << endl;
            if (params.comprimir || params.comprimirYEncriptar) {
                comprimirConDeflate(params.entrada, params.salida);
            } else if (params.descomprimir || params.desencriptarYDescomprimir) {
                descomprimirConDeflate(params.entrada, params.salida);
            }
        } else if (S_ISDIR(entryStat.st_mode)) {
            cout << "Detectado: carpeta" << endl;
            if (params.comprimir || params.comprimirYEncriptar) {
                comprimirCarpeta(params.entrada, params.salida, params.algoritmoComp);
            } else if (params.descomprimir || params.desencriptarYDescomprimir) {
                descomprimirCarpeta(params.entrada, params.salida, params.algoritmoComp);
            }
        } else {
            throw runtime_error("Error: Tipo de entrada no soportado");
        }

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        exit(1);
    }
}