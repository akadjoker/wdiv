
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include "interpreter.hpp"
#include "opcode.hpp"
#include <raylib.h>

Value native_write(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("write expects string as first argument");
        return Value::makeNil();
    }

    const char *fmt = args[0].asString()->chars();
    std::string result;
    int argIndex = 1;

    for (int i = 0; fmt[i] != '\0'; i++)
    {
        if (fmt[i] == '{' && fmt[i + 1] == '}')
        {

            if (argIndex < argCount)
            {

                char buffer[64];
                Value v = args[argIndex++];

                if (v.isInt())
                    snprintf(buffer, 64, "%ld", v.asInt());
                else if (v.isDouble())
                    snprintf(buffer, 64, "%.2f", v.asDouble());
                else if (v.isString())
                    snprintf(buffer, 64, "%s", v.asString()->chars());
                else
                    snprintf(buffer, 64, "<?>");

                result += buffer;
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

Value native_format(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("format expects string as first argument");
        return Value::makeNil();
    }

    const char *fmt = args[0].asString()->chars();
    std::string result;
    int argIndex = 1;

    for (int i = 0; fmt[i] != '\0'; i++)
    {
        if (fmt[i] == '{' && fmt[i + 1] == '}')
        {
            // Substitui {}
            if (argIndex < argCount)
            {
                // Converte Value para string
                char buffer[64];
                Value v = args[argIndex++];

                if (v.isInt())
                    snprintf(buffer, 64, "%ld", v.asInt());
                else if (v.isDouble())
                    snprintf(buffer, 64, "%.2f", v.asDouble());
                else if (v.isString())
                    snprintf(buffer, 64, "%s", v.asString()->chars());
                else
                    snprintf(buffer, 64, "<?>");

                result += buffer;
            }
            i++; // salta '}'
        }
        else
        {
            result += fmt[i];
        }
    }

    return Value::makeString(result.c_str());
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

Value native_key(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1 || !args[0].isInt())
    {
        vm->runtimeError("key expects 1 integer argument");
        return Value::makeNil();
    }

    int key = args[0].asInt();
    bool pressed = IsKeyPressed(key);
    return Value::makeBool(pressed);
}

Value native_mouseX(Interpreter *vm, int argCount, Value *args)
{
    int x = GetMouseX();
    return Value::makeInt(x);
}
Value native_mouseY(Interpreter *vm, int argCount, Value *args)
{
    int y = GetMouseY();
    return Value::makeInt(y);
}

Value native_mouse_down(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1 || !args[0].isInt())
    {
        vm->runtimeError("mouse_down expects 1 integer argument");
        return Value::makeNil();
    }

    int button = args[0].asInt();
    bool down = IsMouseButtonDown(button);
    return Value::makeBool(down);
}

Value native_rand(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1 || !args[0].isInt())
    {
        vm->runtimeError("rand expects 1 integer argument");
        return Value::makeNil();
    }

    int max = args[0].asInt();
    return Value::makeInt(GetRandomValue(0, max));
}

Value native_rand_range(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 2 || !args[0].isInt() || !args[1].isInt())
    {
        vm->runtimeError("rand_range expects 2 integer arguments");
        return Value::makeNil();
    }
    int min = args[0].asInt();
    int max = args[1].asInt();
    return Value::makeInt(GetRandomValue(min, max));
}
Texture2D bunny;

void onStart(Process *proc)
{
    // printf("[start] %s id=%u\n", proc->name->chars(), proc->id);
}

void onUpdate(Process *proc, float dt)
{
    // ler posição vinda do script
   

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
    float x = (float)proc->privates[0].asDouble();
    float y = (float)proc->privates[1].asDouble();

    //DrawCircle((int)x, (int)y, 20, RED);

    DrawTexture(bunny, (int)x - bunny.width / 2, (int)y - bunny.height / 2, WHITE);
}

int main()
{
    Interpreter vm;
    vm.registerNative("write", native_write, -1);
    vm.registerNative("format", native_format, -1);
    vm.registerNative("sqrt", native_sqrt, 1);
    vm.registerNative("clock", native_clock, 0);
    vm.registerNative("sin", native_sin, 1);
    vm.registerNative("cos", native_cos, 1);
    vm.registerNative("abs", native_abs, 1);
    vm.registerNative("key", native_key, 1);
    vm.registerNative("mouseX", native_mouseX, 0);
    vm.registerNative("mouseY", native_mouseY, 0);
    vm.registerNative("mouse_down", native_mouse_down, 1);
    vm.registerNative("rand", native_rand, 1);
    vm.registerNative("rand_range", native_rand_range, 2);

    VMHooks hooks;
    hooks.onStart = onStart;
    hooks.onUpdate = onUpdate;
    hooks.onDestroy = onDestroy;
    hooks.onRender = onRender;

    vm.setHooks(hooks);

    std::ifstream file("bunny.cc");
    std::string code((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

   

    InitWindow(800, 600, "Game");
    SetTargetFPS(60);

    bunny = LoadTexture("assets/wabbit_alpha.png");

     if (!vm.run(code.c_str()))
    {
        std::cerr << "Error running code.\n";

        return 1;
    }

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        vm.update(dt);

        BeginDrawing();
        
        ClearBackground(BLACK);


        vm.render();

        DrawFPS(10, 10);
        DrawText(TextFormat("Processes: %d", vm.getTotalAliveProcesses()), 10, 30, 20, GREEN);

        EndDrawing();
    }

    UnloadTexture(bunny);

    CloseWindow();

    return 0;
}