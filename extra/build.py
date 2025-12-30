#!/usr/bin/env python3
# ============================================
# build.py - BuLang WebAssembly Builder
# Otimizado com cache e build incremental!
# ============================================

import os
import sys
import json
import time
import hashlib
import subprocess
import shutil
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed

# ============================================
# Configuração
# ============================================

CONFIG = {
    "raylib_src": "external/raylib/src",
    "raylib_lib": "external/raylib/src/libraylib.web.a",
    "src_dir": "src",
    "include_dir": "include",
    "assets_dir": "assets",
    "build_dir": "build",
    "cache_file": "build/.build_cache.json",
    "output": "build/bulang.html",
    
    # Flags
    "cxxflags": ["-std=c++17", "-O2", "-Wall"],
    "emflags": [
        "-s", "USE_GLFW=3",
        "-s", "ASYNCIFY",
        "-s", "ALLOW_MEMORY_GROWTH=1",
        "-s", "TOTAL_MEMORY=67108864",
        "-s", "STACK_SIZE=5242880",  # 5MB stack
        "-s", "FORCE_FILESYSTEM=1",
        "--preload-file", "assets@/assets",
        "--shell-file", "shell.html",
        "-s", "EXPORTED_RUNTIME_METHODS=['ccall','cwrap']",
        "-s", "EXPORTED_FUNCTIONS=['_main']",
    ],
    
    # Threads
    "max_workers": 4,
}

# ============================================
# Cores e Emojis
# ============================================

class Colors:
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

def log(emoji, msg, color=Colors.RESET):
    print(f"{color}{emoji} {msg}{Colors.RESET}")

def log_section(title):
    print(f"\n{Colors.BOLD}{Colors.BLUE}{'='*50}{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.BLUE}{title:^50}{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.BLUE}{'='*50}{Colors.RESET}\n")

# ============================================
# Cache Management
# ============================================

class BuildCache:
    def __init__(self, cache_file):
        self.cache_file = cache_file
        self.cache = self._load()
    
    def _load(self):
        if os.path.exists(self.cache_file):
            try:
                with open(self.cache_file, 'r') as f:
                    return json.load(f)
            except:
                return {}
        return {}
    
    def save(self):
        os.makedirs(os.path.dirname(self.cache_file), exist_ok=True)
        with open(self.cache_file, 'w') as f:
            json.dump(self.cache, f, indent=2)
    
    def get_file_hash(self, filepath):
        """Get MD5 hash of file"""
        if not os.path.exists(filepath):
            return None
        
        md5 = hashlib.md5()
        with open(filepath, 'rb') as f:
            for chunk in iter(lambda: f.read(8192), b''):
                md5.update(chunk)
        return md5.hexdigest()
    
    def needs_rebuild(self, source, output, dependencies=None):
        """Check if output needs to be rebuilt"""
        # Output doesn't exist
        if not os.path.exists(output):
            return True
        
        # Source is newer than output
        if os.path.getmtime(source) > os.path.getmtime(output):
            return True
        
        # Check hash cache
        source_hash = self.get_file_hash(source)
        cached = self.cache.get(source, {})
        
        if cached.get('hash') != source_hash:
            return True
        
        # Check dependencies
        if dependencies:
            for dep in dependencies:
                if os.path.exists(dep):
                    if os.path.getmtime(dep) > os.path.getmtime(output):
                        return True
        
        return False
    
    def update(self, source, output):
        """Update cache for a file"""
        self.cache[source] = {
            'hash': self.get_file_hash(source),
            'output': output,
            'timestamp': time.time()
        }

# ============================================
# Build Steps
# ============================================

def check_emscripten():
    """Verify Emscripten is available"""
    log_section("🔍 Verificando Emscripten")
    
    try:
        result = subprocess.run(['emcc', '--version'], 
                              capture_output=True, 
                              text=True,
                              timeout=5)
        version = result.stdout.split('\n')[0]
        log("✅", f"Emscripten encontrado: {version}", Colors.GREEN)
        return True
    except:
        log("❌", "Emscripten não encontrado!", Colors.RED)
        log("ℹ️", "Execute: source /path/to/emsdk/emsdk_env.sh", Colors.YELLOW)
        return False

def check_raylib(cache):
    """Check if Raylib needs to be compiled"""
    log_section("📦 Verificando Raylib")
    
    raylib_src = CONFIG["raylib_src"]
    raylib_lib = CONFIG["raylib_lib"]
    
    # Raylib directory doesn't exist
    if not os.path.exists(raylib_src):
        log("❌", "Raylib não encontrado!", Colors.RED)
        log("ℹ️", "Clonando Raylib...", Colors.YELLOW)
        
        try:
            subprocess.run([
                'git', 'clone', '--depth', '1',
                'https://github.com/raysan5/raylib.git',
                'external/raylib'
            ], check=True)
            log("✅", "Raylib clonado!", Colors.GREEN)
        except subprocess.CalledProcessError:
            log("❌", "Falha ao clonar Raylib!", Colors.RED)
            return False
    
    # Check if libraylib.a exists and is up-to-date
    if os.path.exists(raylib_lib):
        # Check cache usando Makefile como source ao invés do diretório
        makefile_path = os.path.join(raylib_src, "Makefile")
        if os.path.exists(makefile_path):
            if not cache.needs_rebuild(makefile_path, raylib_lib):
                log("✅", "Raylib já compilado (cached)", Colors.GREEN)
                return True
        else:
            # Se não existe Makefile, verifica só se lib existe
            log("✅", "Raylib já compilado (cached)", Colors.GREEN)
            return True
        
        log("ℹ️", "Raylib precisa recompilação", Colors.YELLOW)
    
    return compile_raylib(cache)

def compile_raylib(cache):
    """Compile Raylib for web"""
    log("🔨", "Compilando Raylib para Web...", Colors.BLUE)
    
    raylib_src = CONFIG["raylib_src"]
    
    try:
        result = subprocess.run(
            ['make', 'PLATFORM=PLATFORM_WEB', '-B'],
            cwd=raylib_src,
            capture_output=True,
            text=True,
            timeout=300  # 5 min timeout
        )
        
        if result.returncode == 0:
            log("✅", "Raylib compilado!", Colors.GREEN)
            # Atualizar cache usando Makefile como source
            makefile_path = os.path.join(raylib_src, "Makefile")
            if os.path.exists(makefile_path):
                cache.update(makefile_path, CONFIG["raylib_lib"])
            cache.save()
            return True
        else:
            log("❌", "Erro compilando Raylib!", Colors.RED)
            print(result.stderr)
            return False
            
    except subprocess.TimeoutExpired:
        log("❌", "Timeout compilando Raylib!", Colors.RED)
        return False
    except Exception as e:
        log("❌", f"Erro: {e}", Colors.RED)
        return False

def find_source_files():
    """Find all .cpp files"""
    src_dir = Path(CONFIG["src_dir"])
    return list(src_dir.glob("*.cpp"))

def compile_source_file(source_file, cache):
    """Compile a single .cpp file"""
    src_path = str(source_file)
    obj_path = os.path.join(CONFIG["build_dir"], 
                           source_file.stem + ".o")
    
    # Check if needs rebuild
    include_dir = CONFIG["include_dir"]
    headers = list(Path(include_dir).glob("*.hpp")) if os.path.exists(include_dir) else []
    
    if not cache.needs_rebuild(src_path, obj_path, [str(h) for h in headers]):
        return obj_path, True, "cached"
    
    # Compile
    cmd = ['em++'] + CONFIG["cxxflags"] + [
        f'-I{CONFIG["include_dir"]}',
        f'-I{CONFIG["raylib_src"]}',
        '-c', src_path,
        '-o', obj_path
    ]
    
    try:
        result = subprocess.run(cmd, 
                              capture_output=True, 
                              text=True,
                              timeout=60)
        
        if result.returncode == 0:
            cache.update(src_path, obj_path)
            return obj_path, True, "compiled"
        else:
            return obj_path, False, result.stderr
            
    except Exception as e:
        return obj_path, False, str(e)

def compile_sources(cache):
    """Compile all source files (in parallel)"""
    log_section("📦 Compilando Fontes")
    
    os.makedirs(CONFIG["build_dir"], exist_ok=True)
    
    sources = find_source_files()
    
    if not sources:
        log("❌", f"Nenhum ficheiro .cpp encontrado em {CONFIG['src_dir']}/", Colors.RED)
        return None
    
    log("ℹ️", f"Encontrados {len(sources)} ficheiros .cpp", Colors.BLUE)
    
    objects = []
    compiled = 0
    cached = 0
    errors = []
    
    # Compile in parallel
    with ThreadPoolExecutor(max_workers=CONFIG["max_workers"]) as executor:
        futures = {
            executor.submit(compile_source_file, src, cache): src 
            for src in sources
        }
        
        for future in as_completed(futures):
            src = futures[future]
            obj_path, success, status = future.result()
            
            if success:
                objects.append(obj_path)
                if status == "compiled":
                    log("✅", f"Compilado: {src.name}", Colors.GREEN)
                    compiled += 1
                else:
                    log("💾", f"Cached: {src.name}", Colors.BLUE)
                    cached += 1
            else:
                log("❌", f"Erro: {src.name}", Colors.RED)
                errors.append((src, status))
    
    # Summary
    print()
    log("📊", f"Compilados: {compiled} | Cached: {cached} | Erros: {len(errors)}", 
        Colors.BLUE)
    
    if errors:
        print()
        for src, error in errors:
            log("❌", f"{src.name}:", Colors.RED)
            print(error)
        return None
    
    cache.save()
    return objects

def link_wasm(objects):
    """Link final WebAssembly"""
    log_section("🌐 Linking WebAssembly")
    
    cmd = ['em++'] + objects + [
        CONFIG["raylib_lib"]
    ] + CONFIG["emflags"] + [
        '-o', CONFIG["output"]
    ]
    
    log("ℹ️", "Linking...", Colors.BLUE)
    
    try:
        result = subprocess.run(cmd, 
                              capture_output=True, 
                              text=True,
                              timeout=120)
        
        if result.returncode == 0:
            log("✅", "Link completo!", Colors.GREEN)
            
            # Show generated files
            output_dir = os.path.dirname(CONFIG["output"])
            output_name = Path(CONFIG["output"]).stem
            
            print()
            log("📦", "Ficheiros gerados:", Colors.BLUE)
            for ext in ['.html', '.js', '.wasm', '.data']:
                filepath = os.path.join(output_dir, output_name + ext)
                if os.path.exists(filepath):
                    size = os.path.getsize(filepath) / 1024
                    log("  ", f"- {filepath} ({size:.1f} KB)", Colors.GREEN)
            
            return True
        else:
            log("❌", "Erro no link!", Colors.RED)
            print(result.stderr)
            return False
            
    except Exception as e:
        log("❌", f"Erro: {e}", Colors.RED)
        return False

def run_server():
    """Start local web server"""
    log_section("🚀 Iniciando Servidor Web")
    
    build_dir = CONFIG["build_dir"]
    
    log("ℹ️", f"Servidor em: http://localhost:8000", Colors.BLUE)
    log("ℹ️", f"Abre: http://localhost:8000/bulang.html", Colors.GREEN)
    log("ℹ️", "Pressiona Ctrl+C para parar", Colors.YELLOW)
    
    try:
        subprocess.run(['python3', '-m', 'http.server', '8000'],
                      cwd=build_dir)
    except KeyboardInterrupt:
        log("\n👋", "Servidor parado!", Colors.YELLOW)

# ============================================
# Main Build Function
# ============================================

def build(run_after=False, clean=False):
    """Main build function"""
    start_time = time.time()
    
    print()
    print(f"{Colors.BOLD}{Colors.BLUE}╔══════════════════════════════════════════════╗{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.BLUE}║     🚀 BuLang WebAssembly Builder 🚀       ║{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.BLUE}╚══════════════════════════════════════════════╝{Colors.RESET}")
    
    # Clean
    if clean:
        log_section("🧹 Limpando")
        if os.path.exists(CONFIG["build_dir"]):
            shutil.rmtree(CONFIG["build_dir"])
            log("✅", "Build limpo!", Colors.GREEN)
    
    # Init cache
    cache = BuildCache(CONFIG["cache_file"])
    
    # Check Emscripten
    if not check_emscripten():
        return False
    
    # Check/Compile Raylib
    if not check_raylib(cache):
        return False
    
    # Compile sources
    objects = compile_sources(cache)
    if not objects:
        return False
    
    # Link
    if not link_wasm(objects):
        return False
    
    # Success!
    elapsed = time.time() - start_time
    
    print()
    print(f"{Colors.BOLD}{Colors.GREEN}╔══════════════════════════════════════════════╗{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.GREEN}║            ✅ BUILD COMPLETO! ✅             ║{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.GREEN}╚══════════════════════════════════════════════╝{Colors.RESET}")
    log("⏱️", f"Tempo: {elapsed:.2f}s", Colors.BLUE)
    
    # Run server
    if run_after:
        print()
        run_server()
    
    return True

# ============================================
# CLI
# ============================================

def main():
    import argparse
    
    parser = argparse.ArgumentParser(
        description='BuLang WebAssembly Builder',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Exemplos:
  python3 build.py              Build completo
  python3 build.py --run        Build + servidor web
  python3 build.py --clean      Limpar e rebuild
  python3 build.py --info       Ver configuração
        '''
    )
    
    parser.add_argument('--run', action='store_true',
                       help='Iniciar servidor web após build')
    parser.add_argument('--clean', action='store_true',
                       help='Limpar build antes')
    parser.add_argument('--info', action='store_true',
                       help='Mostrar configuração')
    parser.add_argument('--server', action='store_true',
                       help='Só servidor (sem build)')
    
    args = parser.parse_args()
    
    if args.info:
        log_section("📋 Configuração")
        for key, value in CONFIG.items():
            if not key.startswith('_'):
                print(f"  {key}: {value}")
        return
    
    if args.server:
        run_server()
        return
    
    success = build(run_after=args.run, clean=args.clean)
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()