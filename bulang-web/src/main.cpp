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


 

// ============================================
// Helper: Value to String
// ============================================

static std::string valueToString(const Value &v)
{
    char buffer[256];

    switch (v.type)
    {
    case ValueType::NIL:
        return "nil";
    case ValueType::BOOL:
        return v.as.boolean ? "true" : "false";
    case ValueType::INT:
        snprintf(buffer, 256, "%d", v.as.integer);
        return buffer;
    case ValueType::DOUBLE:
        snprintf(buffer, 256, "%.2f", v.as.number);
        return buffer;
    case ValueType::STRING:
        return v.as.string->chars();
    case ValueType::ARRAY:
        return "[array]";
    case ValueType::MAP:
        return "{map}";
    default:
        return "<object>";
    }
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
    } else if (argCount == 1)
    {
        double value = args[0].isInt() ? (double)args[0].asInt() : args[0].asDouble();
        return Value::makeDouble(RandomGenerator::instance().randFloat(0,  value));
    } else 
    {
        double min = args[0].isInt() ? (double)args[0].asInt() : args[0].asDouble();
        double max = args[1].isInt() ? (double)args[1].asInt() : args[1].asDouble();
        return Value::makeDouble(RandomGenerator::instance().randFloat(min, max));
    }
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

    vm.registerNative("sqrt", native_sqrt, 1);
    vm.registerNative("sin", native_sin, 1);
    vm.registerNative("cos", native_cos, 1);
    vm.registerNative("abs", native_abs, 1);
    vm.registerNative("pow", native_pow, 2);
    vm.registerNative("floor", native_floor, 1);
    vm.registerNative("ceil", native_ceil, 1);

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
