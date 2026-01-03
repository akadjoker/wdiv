
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include "bidings.hpp"
#include "random.hpp"
#include "interpreter.hpp"

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

Value native_rand(Interpreter *vm, int argCount, Value *args)
{

    if (argCount == 0)
    {
        return Value::makeDouble(RandomGenerator::instance().randFloat());
    }
    else if (argCount == 1)
    {
        double value = args[0].toDouble();
        return Value::makeDouble(RandomGenerator::instance().randFloat(0, value));
    }
    else
    {
        double min = args[0].toDouble();
        double max = args[1].toDouble();
        return Value::makeDouble(RandomGenerator::instance().randFloat(min, max));
    }
    return Value::makeNil();
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

    Interpreter vm;

    vm.registerNative("sin", native_sin, 1);
    vm.registerNative("cos", native_cos, 1);
    vm.registerNative("sqrt", native_sqrt, 1);
    vm.registerNative("abs", native_abs, 1);

    vm.registerNative("rand", native_rand, 1);

    vm.registerNative("write", native_write, -1);
    vm.registerNative("format", native_format, -1);
    vm.registerNative("clock", native_clock, 0);

    vm.registerNative("InitWindow", RaylibBindings::native_InitWindow, 3);
    vm.registerNative("CloseWindow", RaylibBindings::native_CloseWindow, 0);
    vm.registerNative("WindowShouldClose", RaylibBindings::native_WindowShouldClose, 0);
    vm.registerNative("SetTargetFPS", RaylibBindings::native_SetTargetFPS, 1);
    vm.registerNative("GetFPS", RaylibBindings::native_GetFPS, 0);
    vm.registerNative("GetFrameTime", RaylibBindings::native_GetFrameTime, 0);
    vm.registerNative("BeginDrawing", RaylibBindings::native_BeginDrawing, 0);
    vm.registerNative("EndDrawing", RaylibBindings::native_EndDrawing, 0);
    vm.registerNative("ClearBackground", RaylibBindings::native_ClearBackground, 1);
    vm.registerNative("DrawFPS", RaylibBindings::native_DrawFps, 2);
    vm.registerNative("DrawPixel", RaylibBindings::native_DrawPixel, 3);
    vm.registerNative("DrawText", RaylibBindings::native_DrawText, 5);
    vm.registerNative("DrawLine", RaylibBindings::native_DrawLine, 4);
    vm.registerNative("DrawCircle", RaylibBindings::native_DrawCircle, 4);
    vm.registerNative("DrawRectangle", RaylibBindings::native_DrawRectangle, 5);
    vm.registerNative("DrawRectangleRec", RaylibBindings::native_DrawRectangleRec, 2);
    vm.registerNative("DrawTexture", RaylibBindings::native_DrawTexture, 4);
    vm.registerNative("LoadTexture", RaylibBindings::native_LoadTexture, 1);
    vm.registerNative("UnloadTexture", RaylibBindings::native_UnloadTexture, 1);
    vm.registerNative("GetMousePosition", RaylibBindings::native_GetMousePosition, 0);
    vm.registerNative("IsMouseButtonDown", RaylibBindings::native_IsMouseButtonDown, 1);
    vm.registerNative("IsMouseButtonPressed", RaylibBindings::native_IsMouseButtonPressed, 1);
    vm.registerNative("IsMouseButtonReleased", RaylibBindings::native_IsMouseButtonReleased, 1);
    vm.registerNative("GetMouseX", RaylibBindings::native_GetMouseX, 0);
    vm.registerNative("GetMouseY", RaylibBindings::native_GetMouseY, 0);

    RaylibBindings::registerColor(vm);
    RaylibBindings::registerRectangle(vm);
    RaylibBindings::registerVector2(vm);
    // RaylibBindings::registerVector3(vm);
    // RaylibBindings::registerCamera2D(vm);

    FileLoaderContext ctx;
    ctx.searchPaths[0] = "./bin";
    ctx.searchPaths[1] = "./scrips";
    ctx.searchPaths[2] = ".";
    ctx.pathCount = 3;
    vm.setFileLoader(multiPathFileLoader, &ctx);

    std::ifstream file("bunny.bu");
    std::string code((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

    if (!vm.run(code.c_str(), false))
    {
        std::cerr << "Error running code.\n";
        return 1;
    }

    // for (int i = 0; i < 5; i++)
    // {
    //     // printf("\n=== FRAME %d ===\n", i);
    //     vm.update(0.016f);

    //     // Pausa para ver output
    //     std::this_thread::sleep_for(std::chrono::milliseconds(16));
    // }

    return 0;
}
