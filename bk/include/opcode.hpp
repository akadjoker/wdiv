#pragma once
#include "config.hpp"

enum Opcode : uint8
{
    // Literals
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,

    // Stack
    OP_POP,
    OP_HALT,
    OP_NOT,
    OP_DUP,

    // Arithmetic
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_MODULO,

    // Bitwise
    OP_BITWISE_AND,
    OP_BITWISE_OR,
    OP_BITWISE_XOR,
    OP_BITWISE_NOT,
    OP_SHIFT_LEFT,
    OP_SHIFT_RIGHT,

    // Comparisons
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_LESS,
    OP_LESS_EQUAL,

    // Variables
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_GET_PRIVATE,
    OP_SET_PRIVATE,

    // Control flow
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_GOSUB,
    OP_RETURN_SUB,

    // Functions
    OP_CALL,
    OP_CALL_NATIVE,
    OP_RETURN,
    OP_RETURN_NIL,
    OP_SPAWN,

    OP_YIELD,
    OP_FRAME,

    OP_EXIT,

    OP_DEFINE_STRUCT,
    OP_DEFINE_ARRAY,
    OP_DEFINE_MAP,

    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_GET_INDEX,
    OP_SET_INDEX,
    
    OP_INVOKE,
    OP_INHERIT,      // Herança
    OP_GET_SUPER,    // super.method
    OP_SUPER_INVOKE, // super.method(args)

    // I/O
    OP_PRINT,
};