
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include "pool.hpp"
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
#include <type_traits>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include "interpreter.hpp"

extern size_t getMemoryUsage();

// Helper: converte Value para string
static void valueToString(const Value &v, std::string &out)
{
    char buffer[256];

    switch (v.type)
    {
    case ValueType::NIL:
        out += "nil";
        break;
    case ValueType::BOOL:
        out += v.as.boolean ? "true" : "false";
        break;
    case ValueType::BYTE:
        snprintf(buffer, 256, "%u", v.as.byte);
        out += buffer;
        break;
    case ValueType::INT:
        snprintf(buffer, 256, "%d", v.as.integer);
        out += buffer;
        break;
    case ValueType::UINT:
        snprintf(buffer, 256, "%u", v.as.unsignedInteger);
        out += buffer;
        break;
    case ValueType::FLOAT:
        snprintf(buffer, 256, "%.2f", v.as.n_float);
        out += buffer;
        break;
    case ValueType::DOUBLE:
        snprintf(buffer, 256, "%.2f", v.as.number);
        out += buffer;
        break;
    case ValueType::STRING:
        out += v.as.string->chars();
        break;
    case ValueType::ARRAY:
        out += "[array]";
        break;
    case ValueType::MAP:
        out += "{map}";
        break;
    default:
        out += "<object>";
    }
}

Value native_format(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || args[0].type != ValueType::STRING)
    {
        vm->runtimeError("format expects string as first argument");
        return Value::makeNil();
    }

    const char *fmt = args[0].as.string->chars();
    std::string result;
    int argIndex = 1;

    for (int i = 0; fmt[i] != '\0'; i++)
    {
        if (fmt[i] == '{' && fmt[i + 1] == '}')
        {
            if (argIndex < argCount)
            {
                valueToString(args[argIndex++], result);
            }
            i++;
        }
        else
        {
            result += fmt[i];
        }
    }

    return Value::makeString(result.c_str());
}

Value native_write(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || args[0].type != ValueType::STRING)
    {
        vm->runtimeError("write expects string as first argument");
        return Value::makeNil();
    }

    const char *fmt = args[0].as.string->chars();
    std::string result;
    int argIndex = 1;

    for (int i = 0; fmt[i] != '\0'; i++)
    {
        if (fmt[i] == '{' && fmt[i + 1] == '}')
        {
            if (argIndex < argCount)
            {
                valueToString(args[argIndex++], result);
            }
            i++;
        }
        else
        {
            result += fmt[i];
        }
    }

    printf("%s", result.c_str());
    return Value::makeNil();
}

Value native_getMemoryUsage(Interpreter *vm, int argCount, Value *args)
{
    return Value::makeInt(getMemoryUsage());
}

 

struct FileLoaderContext
{
    const char *searchPaths[8];
    int pathCount;
    char fullPath[512];
    char buffer[1024 * 1024];
};

const char *multiPathFileLoader(const char *filename, size_t *outSize, void *userdata)
{
    FileLoaderContext *ctx = (FileLoaderContext *)userdata;

    for (int i = 0; i < ctx->pathCount; i++)
    {
        snprintf(ctx->fullPath, sizeof(ctx->fullPath),
                 "%s/%s", ctx->searchPaths[i], filename);

        FILE *f = fopen(ctx->fullPath, "rb");
        if (!f)
        {
            continue; // Tenta próximo path
        }

        // Obtém size
        fseek(f, 0, SEEK_END);
        long size = ftell(f);

        if (size <= 0)
        {
            fprintf(stderr, "Empty or error reading: %s (size=%ld)\n",
                    ctx->fullPath, size);
            fclose(f);
            continue; // Tenta próximo path
        }

        if (size >= sizeof(ctx->buffer))
        {
            fprintf(stderr, "File too large: %s (%ld bytes, max %zu)\n",
                    ctx->fullPath, size, sizeof(ctx->buffer));
            fclose(f);
            *outSize = 0;
            return nullptr;
        }

        fseek(f, 0, SEEK_SET);

        size_t bytesRead = fread(ctx->buffer, 1, size, f);
        fclose(f);

        if (bytesRead != (size_t)size)
        {
            fprintf(stderr, "Read error: %s (expected %ld, got %zu)\n",
                    ctx->fullPath, size, bytesRead);
            continue;
        }

        ctx->buffer[bytesRead] = '\0';

        printf("✓ Loaded: %s (%zu bytes)\n", ctx->fullPath, bytesRead);

        *outSize = bytesRead;
        return ctx->buffer;
    }

    fprintf(stderr, "✗ File not found in any path: %s\n", filename);
    fprintf(stderr, "  Searched paths:\n");
    for (int i = 0; i < ctx->pathCount; i++)
    {
        fprintf(stderr, "    - %s/%s\n", ctx->searchPaths[i], filename);
    }

    *outSize = 0;
    return nullptr;
}

struct Color
{
    unsigned char r, g, b, a;
};

// ========================================
// COLOR
// ========================================

void color_ctor(Interpreter *vm, void *buffer, int argc, Value *args)
{
    Color *v = (Color *)buffer;
    v->r = args[0].asByte();
    v->g = args[1].asByte();
    v->b = args[2].asByte();
    v->a = args[3].asByte();
 //   Info("Create Color(%d, %d, %d, %d)", v->r, v->g, v->b, v->a);
}

void color_dtor(Interpreter *vm, void *buffer)
{
    Color *v = (Color *)buffer;

   // Info("Delet Color(%d, %d, %d, %d)", v->r, v->g, v->b, v->a);
}

void registerColor(Interpreter &vm)
{
    auto *color = vm.registerNativeStruct(
        "Color",
        sizeof(Color),
        color_ctor,
        color_dtor);

    vm.addStructField(color, "r", offsetof(Color, r), FieldType::BYTE);
    vm.addStructField(color, "g", offsetof(Color, g), FieldType::BYTE);
    vm.addStructField(color, "b", offsetof(Color, b), FieldType::BYTE);
    vm.addStructField(color, "a", offsetof(Color, a), FieldType::BYTE);
}

struct Sprite
{
    int id;
    int x, y;
    int graph;
    const char *name;
};

int nextSpriteId = 0;

// ========================================
// SPRITE.CPP - COM VM
// ========================================

int count =0;


void *sprite_constructor(Interpreter *vm, int argCount, Value *args)
{
    Sprite *sprite = new Sprite();
    sprite->id = nextSpriteId++;
    sprite->graph = args[0].asInt();
    sprite->x = args[1].asInt();
    sprite->y = args[2].asInt();
    sprite->name = args[3].asString()->chars();

    count++;

  //  Info("Creating sprite: %s (%d, %d)", sprite->name, sprite->x, sprite->y);

    return sprite;
}

void sprite_destructor(Interpreter *vm, void *instance)
{
    Sprite *sprite = (Sprite *)instance;

   // Info("Destroying sprite: %s\n", sprite->name);

    count--;

    delete sprite;
}

Value sprite_move(Interpreter *vm, void *instance, int argCount, Value *args)
{
    if (argCount != 2)
    {
        vm->runtimeError("move() expects 2 arguments");
        return Value::makeNil();
    }

    Sprite *sprite = (Sprite *)instance;
    sprite->x += args[0].asInt();
    sprite->y += args[1].asInt();

    printf("Sprite moved: %s (%d, %d)\n", sprite->name, sprite->x, sprite->y);

    return Value::makeNil();
}
Value sprite_draw(Interpreter *vm, void *instance, int argCount, Value *args)
{
    Sprite *sprite = (Sprite *)instance;

    printf("Drawing sprite: %s (%d, %d)\n", sprite->name, sprite->x, sprite->y);

    return Value::makeNil();
}
Value sprite_setPos(Interpreter *vm, void *instance, int argCount, Value *args)
{
    Sprite *sprite = (Sprite *)instance;

    sprite->x = args[0].asInt();
    sprite->y = args[1].asInt();

    printf("Sprite moved: %s (%d, %d)\n", sprite->name, sprite->x, sprite->y);

    return Value::makeNil();
}

Value sprite_getName(Interpreter *vm, void *instance, int argCount, Value *args)
{
    Sprite *sprite = (Sprite *)instance;
    return Value::makeString(sprite->name);
}

Value sprite_get_x(Interpreter *vm, void *instance)
{
    Sprite *sprite = (Sprite *)instance;
    return Value::makeInt(sprite->x);
}

// Setter: x
void sprite_set_x(Interpreter *vm, void *instance, Value value)
{
    Sprite *sprite = (Sprite *)instance;
    sprite->x = value.asInt();
}

// Getter: y
Value sprite_get_y(Interpreter *vm, void *instance)
{
    Sprite *sprite = (Sprite *)instance;
    return Value::makeInt(sprite->y);
}

// Setter: y
void sprite_set_y(Interpreter *vm, void *instance, Value value)
{
    Sprite *sprite = (Sprite *)instance;
    sprite->y = value.asInt();
}

// Getter: id (read-only)
Value sprite_get_id(Interpreter *vm, void *instance)
{
    Sprite *sprite = (Sprite *)instance;
    return Value::makeInt(sprite->id);
}

void registerSpriteClass(Interpreter &vm)
{
    NativeClassDef *spriteClass = vm.registerNativeClass(
        "Sprite",
        sprite_constructor,
        sprite_destructor,
        4 // argCount
    );

    vm.addNativeMethod(spriteClass, "draw", sprite_draw);
    vm.addNativeMethod(spriteClass, "move", sprite_move);
    vm.addNativeMethod(spriteClass, "setPos", sprite_setPos);
    vm.addNativeProperty(spriteClass, "x", sprite_get_x, sprite_set_x);
    vm.addNativeProperty(spriteClass, "y", sprite_get_y, sprite_set_y);
    vm.addNativeProperty(spriteClass, "id", sprite_get_id); // Read-only
}

int main()
{

    {
    Interpreter vm;
    // vm.registerNative("write", native_write, -1);
    // vm.registerNative("getMemoryUsage", native_getMemoryUsage, -1);
    // vm.registerNative("format", native_format, -1);

    registerColor(vm);
    registerSpriteClass(vm);

    FileLoaderContext ctx;
    ctx.searchPaths[0] = "./bin";
    ctx.searchPaths[1] = "./scrips";
    ctx.searchPaths[2] = ".";
    ctx.pathCount = 3;
    vm.setFileLoader(multiPathFileLoader, &ctx);

    std::ifstream file("main.cc");
    std::string code((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
{
    if (!vm.run(code.c_str(), false))
    {
        std::cerr << "Error running code.\n";
        return 1;
    }
}
}

    printf("=== total %d ===\n" , count);
    

    // for (int i = 0; i < 5; i++)
    // {
    //     // printf("\n=== FRAME %d ===\n", i);
    //     vm.update(0.016f);

    //     // Pausa para ver output
    //     std::this_thread::sleep_for(std::chrono::milliseconds(16));
    // }

    return 0;
}
