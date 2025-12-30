# ============================================
# Makefile.web - BuLang WebAssembly Build
# ============================================

PROJECT_ROOT=/media/djoker/code/projects/cpp/wdiv
RAYLIB_PATH=$(PROJECT_ROOT)/game/external/raylib
SRC_DIR=$(PROJECT_ROOT)/game/src
INCLUDE_DIR=$(PROJECT_ROOT)/game/include
ASSETS_DIR=$(PROJECT_ROOT)/assets

# Emscripten compiler
CXX = em++
CC = emcc

# Compiler flags
CXXFLAGS = -std=c++17 -O2 -Wall
CXXFLAGS += -I$(INCLUDE_DIR)
CXXFLAGS += -I$(RAYLIB_PATH)/src

# Emscripten specific flags
EMFLAGS = -s USE_GLFW=3
EMFLAGS += -s ASYNCIFY
EMFLAGS += -s ALLOW_MEMORY_GROWTH=1
EMFLAGS += -s TOTAL_MEMORY=67108864
EMFLAGS += -s FORCE_FILESYSTEM=1
EMFLAGS += --preload-file $(ASSETS_DIR)@/assets
EMFLAGS += --shell-file shell.html
EMFLAGS += -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']
EMFLAGS += -s EXPORTED_FUNCTIONS=['_main']

# Raylib library (compile primeiro!)
RAYLIB_LIB = $(RAYLIB_PATH)/src/libraylib.a

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

# Output
OUTPUT = bulang

.PHONY: all clean raylib

all: raylib $(OUTPUT).html

# Compile Raylib para Web
raylib:
	@echo "🔨 Compilando Raylib para Web..."
	cd $(RAYLIB_PATH)/src && $(MAKE) PLATFORM=PLATFORM_WEB -B

# Compile BuLang para Web
$(OUTPUT).html: $(OBJECTS)
	@echo "🌐 Linking para WebAssembly..."
	$(CXX) $(OBJECTS) $(RAYLIB_LIB) $(EMFLAGS) -o $(OUTPUT).html

# Compile object files
%.o: %.cpp
	@echo "📦 Compilando $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run local web server
run: $(OUTPUT).html
	@echo "🚀 Iniciando servidor web em http://localhost:8000"
	@echo "   Pressiona Ctrl+C para parar"
	python3 -m http.server 8000

# Alternative: usar emrun
emrun: $(OUTPUT).html
	emrun --no_browser --port 8000 $(OUTPUT).html

# Clean
clean:
	@echo "🧹 Limpando..."
	rm -f $(OBJECTS)
	rm -f $(OUTPUT).html $(OUTPUT).js $(OUTPUT).wasm $(OUTPUT).data
	cd $(RAYLIB_PATH)/src && $(MAKE) clean

# Clean tudo incluindo Raylib
clean-all: clean
	@echo "🧹 Limpando Raylib..."
	cd $(RAYLIB_PATH)/src && $(MAKE) clean

# Info
info:
	@echo "📋 Informação do Build:"
	@echo "  Projeto: $(PROJECT_ROOT)"
	@echo "  Raylib: $(RAYLIB_PATH)"
	@echo "  Sources: $(SRC_DIR)"
	@echo "  Include: $(INCLUDE_DIR)"
	@echo "  Assets: $(ASSETS_DIR)"
	@echo ""
	@echo "  Fontes encontradas: $(words $(SOURCES))"
	@echo "  $(SOURCES)"

# Help
help:
	@echo "🌐 BuLang WebAssembly Build"
	@echo ""
	@echo "Comandos disponíveis:"
	@echo "  make -f Makefile.web         - Build completo"
	@echo "  make -f Makefile.web raylib  - Só Raylib"
	@echo "  make -f Makefile.web run     - Build + servidor web"
	@echo "  make -f Makefile.web clean   - Limpar build"
	@echo "  make -f Makefile.web info    - Ver configuração"
	@echo ""
	@echo "Depois de compilar, abre: http://localhost:8000/bulang.html"
