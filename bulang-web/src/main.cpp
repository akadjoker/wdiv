#include <iostream>
#include <string>
#include <cmath>
#include <emscripten.h>
#include <emscripten/bind.h>
#include "platform.hpp"
#include "interpreter.hpp"
#include "Outputcapture.h"
#include "webapi.h"
#include "random.hpp"

OutputCapture *g_currentOutput = nullptr;

 

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
        double value = args[0].isInt() ? (double)args[0].asInt() : args[0].asDouble();
        return Value::makeDouble(RandomGenerator::instance().randFloat(0, value));
    }
    else
    {
        double min = args[0].isInt() ? (double)args[0].asInt() : args[0].asDouble();
        double max = args[1].isInt() ? (double)args[1].asInt() : args[1].asDouble();
        return Value::makeDouble(RandomGenerator::instance().randFloat(min, max));
    }
    return Value::makeNil();
}


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

    OsPrintf("%s", result.c_str());
    return Value::makeNil();
}


Value native_sqrt(Interpreter *vm, int argCount, Value *args)
{
    if (argCount != 1)
    {
        vm->runtimeError("sqrt expects 1 argument");
        return Value::makeNil();
    }

    double value = args[0].isInt() ? (double)args[0].asInt() : args[0].asDouble();
    return Value::makeDouble(std::sqrt(value));
}

Value native_sin(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1)
    {
        vm->runtimeError("sin expects 1 argument");
        return Value::makeNil();
    }
    double x = args[0].isInt() ? (double)args[0].asInt() : args[0].asDouble();
    return Value::makeDouble(std::sin(x));
}

Value native_cos(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1)
    {
        vm->runtimeError("cos expects 1 argument");
        return Value::makeNil();
    }
    double x = args[0].isInt() ? (double)args[0].asInt() : args[0].asDouble();
    return Value::makeDouble(std::cos(x));
}

Value native_abs(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1)
    {
        vm->runtimeError("abs expects 1 argument");
        return Value::makeNil();
    }

    if (args[0].isInt())
        return Value::makeInt(std::abs(args[0].asInt()));
    else
        return Value::makeDouble(std::fabs(args[0].asDouble()));
}

Value native_pow(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2)
    {
        vm->runtimeError("pow expects 2 arguments");
        return Value::makeNil();
    }

    double base = args[0].isInt() ? (double)args[0].asInt() : args[0].asDouble();
    double exp = args[1].isInt() ? (double)args[1].asInt() : args[1].asDouble();
    return Value::makeDouble(std::pow(base, exp));
}

Value native_floor(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1)
    {
        vm->runtimeError("floor expects 1 argument");
        return Value::makeNil();
    }

    double value = args[0].isInt() ? (double)args[0].asInt() : args[0].asDouble();
    return Value::makeInt((int)std::floor(value));
}

Value native_ceil(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1)
    {
        vm->runtimeError("ceil expects 1 argument");
        return Value::makeNil();
    }

    double value = args[0].isInt() ? (double)args[0].asInt() : args[0].asDouble();
    return Value::makeInt((int)std::ceil(value));
}

// ============================================
// Execute Code (exposto ao JS)
// ============================================

std::string executeCode(const std::string &code)
{
    OutputCapture output;
    g_currentOutput = &output; // Ativa captura - OsPrintf vai usar isto!

    Interpreter vm;

        vm.addModule("math")
        .addDouble("PI", 3.14159265358979)
        .addDouble("E", 2.71828182845905)
        .addFloat("SQRT2", 1.41421356f)
        .addInt("MAX_INT", 2147483647)
        .addFunction("sin",  native_sin , 1)
        .addFunction("cos",  native_cos , 1)
        .addFunction("sqrt", native_sqrt, 1)
        .addFunction("abs",  native_abs , 1)
        .addFunction("rand", native_rand, -1);
 
    vm.registerNative("clock", native_clock, 0);
    vm.registerNative("write", native_write, -1);
    vm.registerNative("format", native_format, -1);

    registerWebNatives(&vm);

    std::string result;

    try
    {

        if (!vm.run(code.c_str(), false))
        {

            std::string compileError = output.getOutput();
            if (compileError.empty())
            {
                result = "❌ Compilation Error";
            }
            else
            {
                result = compileError;
            }
        }
        else
        {
            // Sucesso
            result = output.getOutput();
            if (result.empty())
            {
                result = "✓ Executed successfully (no output)";
            }
        }
    }
    catch (const std::exception &e)
    {

        result = std::string("❌ Runtime Error: ") + e.what();
        std::string vmOutput = output.getOutput();
        if (!vmOutput.empty())
        {
            result += ": \n" + vmOutput;
        }
    }

    g_currentOutput = nullptr;
    return result;
}

// ============================================
// Embind Bindings
// ============================================

EMSCRIPTEN_BINDINGS(bulang_playground)
{
    emscripten::function("executeCode", &executeCode);
}
