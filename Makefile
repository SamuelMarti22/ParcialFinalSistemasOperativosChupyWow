# Makefile para el proyecto de Compresión y Encriptación
# Autor: Sistema de Compresión con ChaCha20 y Deflate

# Compilador y flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -fopenmp
TARGET = ejecuta

# Archivos fuente (listados directamente para evitar problemas con paréntesis en nombres)
SOURCES = main.cpp \
          comandos.cpp \
          likeDeflate/main.cpp \
          likeDeflate/lz77.cpp \
          likeDeflate/huffman.cpp \
          likeDeflate/chupy_header.cpp \
          likeDeflate/folder_compressor.cpp

# Archivos de ChaCha20 (separados por el problema de paréntesis en el nombre)
CHACHA_SOURCES = ChaCha20(encriptacion)/ChaCha20.cpp \
                 ChaCha20(encriptacion)/sha256.cpp

# Todos los archivos fuente
ALL_SOURCES = $(SOURCES) $(CHACHA_SOURCES)

# Headers (para dependencias)
HEADERS = comandos.h \
          likeDeflate/deflate_interface.h \
          likeDeflate/lz77.h \
          likeDeflate/huffman.h \
          likeDeflate/chupy_header.h \
          likeDeflate/folder_compressor.h \
          ChaCha20(encriptacion)/ChaCha20.h \
          ChaCha20(encriptacion)/sha256.h

# Regla principal
all: $(TARGET)
	@printf "\033[32m✓ Compilación completada exitosamente\033[0m\n"
	@printf "\033[34mEjecuta con: ./$(TARGET)\033[0m\n"

# Enlazar el ejecutable (compilación directa sin objetos intermedios)
$(TARGET): $(ALL_SOURCES) $(HEADERS)
	@printf "\033[33m→ Compilando y enlazando $(TARGET)...\033[0m\n"
	$(CXX) $(CXXFLAGS) -o "$@" $(SOURCES) "ChaCha20(encriptacion)/ChaCha20.cpp" "ChaCha20(encriptacion)/sha256.cpp"

# Limpiar archivos generados
clean:
	@printf "\033[33m→ Limpiando archivos temporales...\033[0m\n"
	@rm -f *.o likeDeflate/*.o
	@printf "\033[32m✓ Archivos temporales eliminados\033[0m\n"

# Limpiar todo (incluido el ejecutable)
distclean: clean
	@printf "\033[33m→ Eliminando ejecutable...\033[0m\n"
	@rm -f $(TARGET)
	@printf "\033[32m✓ Proyecto limpio\033[0m\n"

# Recompilar desde cero
rebuild: distclean all

# Compilar con información de debug
debug: CXXFLAGS += -g -DDEBUG
debug: clean all
	@printf "\033[32m✓ Compilación con símbolos de debug completada\033[0m\n"

# Mostrar información del proyecto
info:
	@printf "\033[34m════════════════════════════════════════════════════════════\033[0m\n"
	@printf "\033[34m  Proyecto: Sistema de Compresión y Encriptación\033[0m\n"
	@printf "\033[34m════════════════════════════════════════════════════════════\033[0m\n"
	@printf "Compilador:    $(CXX)\n"
	@printf "Flags:         $(CXXFLAGS)\n"
	@printf "Ejecutable:    $(TARGET)\n"
	@printf "Archivos:      $(words $(ALL_SOURCES)) archivos fuente\n"
	@printf "\n"
	@printf "\033[33mComandos disponibles:\033[0m\n"
	@printf "  make           - Compila el proyecto\n"
	@printf "  make clean     - Elimina archivos objeto\n"
	@printf "  make distclean - Elimina todo (objetos + ejecutable)\n"
	@printf "  make rebuild   - Recompila desde cero\n"
	@printf "  make debug     - Compila con símbolos de debug\n"
	@printf "  make info      - Muestra esta información\n"
	@printf "  make help      - Muestra ayuda de uso\n"
	@printf "\033[34m════════════════════════════════════════════════════════════\033[0m\n"

# Ayuda de uso del programa
help: info
	@printf "\n"
	@printf "\033[33mEjemplos de uso del programa:\033[0m\n"
	@printf "\n"
	@printf "\033[32m1. Comprimir archivo:\033[0m\n"
	@printf "   ./$(TARGET) -c -i archivo.txt -o archivo.chupy --comp-alg huffman\n"
	@printf "\n"
	@printf "\033[32m2. Encriptar archivo:\033[0m\n"
	@printf "   ./$(TARGET) -e -i archivo.txt -o archivo.enc --enc-alg chacha20 -k miPassword\n"
	@printf "\n"
	@printf "\033[32m3. Comprimir + Encriptar:\033[0m\n"
	@printf "   ./$(TARGET) -ce -i carpeta -o carpeta.chupy --comp-alg huffman --enc-alg chacha20 -k miPassword\n"
	@printf "\n"
	@printf "\033[32m4. Desencriptar + Descomprimir:\033[0m\n"
	@printf "   ./$(TARGET) -ud -i carpeta.chupy -o carpeta_recuperada --comp-alg huffman --enc-alg chacha20 -k miPassword\n"
	@printf "\n"

# Declarar targets que no son archivos
.PHONY: all clean distclean rebuild debug info help
