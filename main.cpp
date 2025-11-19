#include <iostream>
#include <stdexcept>
#include "comandos.h"

int main(int argc, char* argv[]) {
    try {
        // Leer y validar parámetros
        Parametros params = leerYValidarComandos(argc, argv);
        
        // Ejecutar la operación solicitada
        ejecutarOperacion(params);
        
        std::cout << "Operación completada exitosamente." << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Error desconocido." << std::endl;
        return 1;
    }
}