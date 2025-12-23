
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include "interpreter.hpp"
#include "token.hpp"
#include "lexer.hpp"


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

void onStart(Process *proc)
{
    // printf("[start] %s id=%u\n", proc->name->chars(), proc->id);
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
    printf("[destroy] %s exit=%d\n", proc->name->chars(), proc->exitCode);
}

void onRender(Process *proc)
{
    printf("[render] %s rendering...\n", proc->name->chars());
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

    VMHooks hooks;
    hooks.onStart = onStart;
    hooks.onUpdate = onUpdate;
    hooks.onDestroy = onDestroy;
    hooks.onRender = onRender;

    vm.setHooks(hooks);

    std::ifstream file("main.cc");
    std::string code((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

    if (!vm.run(code.c_str()),true)
    {
        std::cerr << "Error running code.\n";
        return 1;
    }

    int stapes = 0;
    while (stapes < 50)
    {
        stapes++;
        vm.update(0.016f); // Simula um frame de 16ms
    }

    // Lexer lex("3.14 name.upper() 3.method()");

    // Token t;
    // while ((t = lex.scanToken()).type != TOKEN_EOF)
    // {
    //     printf("[%d:%s] ", t.type, t.lexeme.c_str());
    // }

    return 0;
}