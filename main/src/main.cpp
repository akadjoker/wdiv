
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <thread>

#include "interpreter.hpp"
#include "token.hpp"
#include "lexer.hpp"

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
        snprintf(buffer, 256, "%.2f", v.as.real);
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

Value native_sqrt(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1)
    {
        vm->runtimeError("sqrt expects 1 argument");
        return Value::makeNil();
    }

    double value;
    if (args[0].isInt())
        value = (double)args[0].asInt();
    else if (args[0].isDouble())
        value = args[0].asDouble();
    else
    {
        vm->runtimeError("sqrt expects a number");
        return Value::makeNil();
    }

    if (value < 0)
    {
        vm->runtimeError("sqrt of negative number");
        return Value::makeNil();
    }

    return Value::makeDouble(std::sqrt(value));
}

Value native_sin(Interpreter *vm, int argCount, Value *args)
{
    double x = args[0].isInt() ? (double)args[0].asInt() : args[0].asDouble();
    return Value::makeDouble(std::sin(x));
}

Value native_cos(Interpreter *vm, int argCount, Value *args)
{
    double x = args[0].isInt() ? (double)args[0].asInt() : args[0].asDouble();
    return Value::makeDouble(std::cos(x));
}

Value native_abs(Interpreter *vm, int argCount, Value *args)
{
    if (args[0].isInt())
        return Value::makeInt(std::abs(args[0].asInt()));
    else
        return Value::makeDouble(std::fabs(args[0].asDouble()));
}

Value native_clock(Interpreter *vm, int argCount, Value *args)
{
    return Value::makeDouble(static_cast<double>(clock()) / CLOCKS_PER_SEC);
}

struct Vector2
{
    double x, y;
    String* name;
};


void vector2_ctor(Interpreter *vm, void *buffer, int argc, Value *args)
{
    Vector2 *v = (Vector2 *)buffer;
    v->x = args[0].asDouble();
    v->y = args[1].asDouble();
    v->name = args[2].asString();

    Info("Vector2 ctor %f %f %s", v->x, v->y, v->name->chars());
 
}

void vector2_dtor(Interpreter* vm, void* buffer)
{
    Vector2 *v = (Vector2 *)buffer;
 
}

void registerVector2(Interpreter &vm)
{
    auto *vec2 = vm.registerNativeStruct(
        "Vector2",
        sizeof(Vector2),
        vector2_ctor,
        vector2_dtor // Destructor
    );
    vm.addStructField(vec2, "x", offsetof(Vector2, x), FieldType::FLOAT);
    vm.addStructField(vec2, "y", offsetof(Vector2, y), FieldType::FLOAT);
    vm.addStructField(vec2, "name", offsetof(Vector2, name), FieldType::STRING);
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

void *sprite_constructor(Interpreter *vm, int argCount, Value *args)
{
    Sprite *sprite = new Sprite();
    sprite->id = nextSpriteId++;
    sprite->graph = args[0].asInt();
    sprite->x = args[1].asInt();
    sprite->y = args[2].asInt();
    sprite->name = args[3].asString()->chars();

    return sprite;
}

void sprite_destructor(Interpreter *vm, void *instance)
{
    Sprite *sprite = (Sprite *)instance;

    printf("Destroying sprite: %s\n", sprite->name);

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

void onStart(Process *proc)
{
    //  printf("[start] %s id=%u\n", proc->name->chars(), proc->id);
}

void onUpdate(Process *proc, float dt)
{
    // ler posição vinda do script
    float x = (float)proc->privates[0].asDouble();
    float y = (float)proc->privates[1].asDouble();

    // printf("[update] %s pos=(%.2f, %.2f) dt=%.3f\n",
    //     proc->name->chars(), x, y, dt);

    // proc->privates[0] = Value::makeDouble(1);
    // proc->privates[1] = Value::makeDouble(1);
}

void onDestroy(Process *proc, int exitCode)
{
    // printf("[destroy] %s exit=%d\n", proc->name->chars(), proc->exitCode);
}

void onRender(Process *proc)
{
    // printf("[render] %s rendering...\n", proc->name->chars());
}

static char fileBuffer[1024 * 1024];

const char *desktopFileLoader(const char *filename, size_t *outSize, void *userdata)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        *outSize = 0;
        return nullptr;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    fread(fileBuffer, 1, size, f);
    fclose(f);

    *outSize = size;
    return fileBuffer;
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

int main()
{

    // Lexer lex("close:");

    // Token t;
    // while ((t = lex.scanToken()).type != TOKEN_EOF)
    // {
    //     printf("%s \n", t.toString().c_str());
    // }

    // return 0;

    Interpreter vm;
    vm.registerNative("write", native_write, -1);
    vm.registerNative("format", native_format, -1);
    vm.registerNative("sqrt", native_sqrt, 1);
    vm.registerNative("clock", native_clock, 0);
    vm.registerNative("sin", native_sin, 1);
    vm.registerNative("cos", native_cos, 1);
    vm.registerNative("abs", native_abs, 1);
    // vm.setFileLoader(desktopFileLoader, nullptr);

    registerSpriteClass(vm);
    registerVector2(vm);

    FileLoaderContext ctx;
    ctx.searchPaths[0] = "./bin";
    ctx.searchPaths[1] = "./scrips";
    ctx.searchPaths[2] = ".";
    ctx.pathCount = 3;
    vm.setFileLoader(multiPathFileLoader, &ctx);

    VMHooks hooks;
    hooks.onStart = onStart;
    hooks.onUpdate = onUpdate;
    hooks.onDestroy = onDestroy;
    hooks.onRender = onRender;

    vm.setHooks(hooks);

    std::ifstream file("main.cc");
    std::string code((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

    if (!vm.run(code.c_str(), false))
    {
        std::cerr << "Error running code.\n";
        return 1;
    }

    //     vm.pushInt(10);
    //     vm.pushInt(20);

    //  if (vm.callFunction("add", 2)) {
    //     printf("Stack size: %d\n", vm.getTop());  // Deve ser 1

    //     if (vm.getTop() > 0) {
    //         int result = vm.toInt(-1);
    //         printf("Result: %d\n", result);  //  Deve dar 30!
    //         vm.pop();
    //     }
    // } else {
    //     printf("Call failed!\n");
    // }

    // vm.pushInt(100);
    // vm.pushInt(200);
    // vm.callProcess("enemy", 2);

    // vm.pushInt(100);
    // vm.pushInt(200);
    // vm.callProcess("enemy", 2);

    // vm.pushInt(100);
    // vm.pushInt(200);
    // vm.callProcess("enemy", 2);

    for (int i = 0; i < 5; i++)
    {
        // printf("\n=== FRAME %d ===\n", i);
        vm.update(0.016f);

        // Pausa para ver output
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // int stapes = 0;
    // // while (vm.liveProcess()>0)
    // while (stapes < 80000)
    // {
    //     stapes++;
    //     vm.update(0.0000016f); // Simula um frame de 16ms
    // }

    // Lexer lex("3.14 name.upper() 3.method()");

    // Token t;
    // while ((t = lex.scanToken()).type != TOKEN_EOF)
    // {
    //     printf("[%d:%s] ", t.type, t.lexeme.c_str());
    // }

    return 0;
}
