#ifndef COMANDOS_H
#define COMANDOS_H

#include <string>
#include <vector>
#include <cstdint>

using namespace std;

// Estructura que guarda todos los datos que el usuario escribió en la terminal
struct Parametros {
    bool comprimir = false;        // Si el usuario escribió -c
    bool descomprimir = false;     // Si el usuario escribió -d
    bool encriptar = false;        // Si el usuario escribió -e
    bool desencriptar = false;     // Si el usuario escribió -u
    bool comprimirYEncriptar = false;   // Se activa con -ce para comprimir y encriptar
    bool desencriptarYDescomprimir = false; // Se activa con -ud para desencriptar y descomprimir

    string algoritmoComp;     // Nombre del algoritmo de compresión 
    string algoritmoEnc;      // Nombre del algoritmo de encriptación

    string entrada;           // Ruta del archivo/ carpeta de entrada
    string salida;            // Ruta del archivo/ carpeta de salida

    string clave;             // Clave para encriptar
};

// Lee, valida y retorna parámetros, si hay algún error, muestra el mensaje y termina el programa.
Parametros leerYValidarComandos(int argc, char* argv[]);

// Funciones auxiliares
void mostrarAyuda();
vector<uint8_t> leerArchivoConSyscalls(const string& rutaArchivo);
void escribirArchivoConSyscalls(const string& rutaArchivo, const vector<uint8_t>& datos);


#endif
