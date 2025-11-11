#ifndef DEFLATE_INTERFACE_H
#define DEFLATE_INTERFACE_H

#include <string>

// Funciones públicas para usar desde comandos.cpp temp
void comprimirConDeflate(const std::string& archivoEntrada, const std::string& archivoSalida);
void descomprimirConDeflate(const std::string& archivoEntrada, const std::string& archivoSalida);

#endif