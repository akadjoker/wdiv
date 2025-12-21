#pragma once

#include "config.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include <vector>
#include <cstring>
#include <string>

class Code;
struct Value;
class Compiler;
struct Function;
struct CallFrame;
struct Fiber;
struct Process;
class Interpreter;


typedef void (Compiler::*ParseFn)(bool canAssign);

enum Precedence
{
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY
};

struct ParseRule
{
    ParseFn prefix;
    ParseFn infix;
    Precedence prec;
};

#define MAX_IDENTIFIER_LENGTH 32
#define MAX_LOCALS 256

struct Local
{
    char name[MAX_IDENTIFIER_LENGTH];
    uint8 length;
    int depth;

    Local() : length(0), depth(-1)
    {
        name[0] = '\0';
    }

    bool equals(const std::string &str) const
    {

        if (length != str.length())
        {
            return false;
        }

        return std::memcmp(name, str.c_str(), length) == 0;
    }

    bool equals(const char *str, size_t len) const
    {
        if (length != len)
        {
            return false;
        }
        return std::memcmp(name, str, length) == 0;
    }
};

#define MAX_LOOP_DEPTH 32
#define MAX_BREAKS_PER_LOOP 256

struct LoopContext
{
    int loopStart;
    int breakJumps[MAX_BREAKS_PER_LOOP];
    int breakCount;
    int scopeDepth;

    LoopContext() : loopStart(0), breakCount(0), scopeDepth(0) {}

    bool addBreak(int jump)
    {
        if (breakCount >= MAX_BREAKS_PER_LOOP)
        {

            return false;
        }
        breakJumps[breakCount++] = jump;
        return true;
    }
};

#define MAX_LOCALS 256
class Compiler
{
public:
    Compiler(Interpreter *vm);
    ~Compiler();

    Function *compile(const std::string &source, Interpreter *vm);
    Function *compileExpression(const std::string &source, Interpreter *vm);
    Process *compile(const std::string &source);
    Process *compileExpression(const std::string &source);

    void clear();

private:
    Interpreter *vm_;
    Lexer *lexer;
    Token current;
    Token previous;

    Function *function;
    Code *currentChunk;
    Fiber *currentFiber;
    Process *currentProcess;

    bool hadError;
    bool panicMode;

    int scopeDepth;
    Local locals_[MAX_LOCALS];
    int localCount_;

    LoopContext loopContexts_[MAX_LOOP_DEPTH];
    int loopDepth_;
    bool isProcess_;
    // Token management
    void advance();
    bool check(TokenType type);
    bool match(TokenType type);
    void consume(TokenType type, const char *message);

    void beginLoop(int loopStart);
    void endLoop();
    void emitBreak();
    void emitContinue();

    void breakStatement();
    void continueStatement();

    // Error handling
    void error(const char *message);
    void errorAt(Token &token, const char *message);
    void errorAtCurrent(const char *message);
    void synchronize();

    // Bytecode emission
    void emitByte(uint8 byte);
    void emitBytes(uint8 byte1, uint8 byte2);
    void emitReturn();
    void emitConstant(Value value);
    uint8 makeConstant(Value value);

    int emitJump(uint8 instruction);
    void patchJump(int offset);
    void emitLoop(int loopStart);

    // Pratt parser
    void expression();
    void parsePrecedence(Precedence precedence);
    ParseRule *getRule(TokenType type);

    // Parse functions (prefix)
    void number(bool canAssign);
    void string(bool canAssign);
    void literal(bool canAssign);
    void grouping(bool canAssign);
    void unary(bool canAssign);
    void variable(bool canAssign);

    // Parse functions (infix)
    void binary(bool canAssign);
    void and_(bool canAssign);
    void or_(bool canAssign);
    void call(bool canAssign);

    // Statements
    void declaration();
    void statement();
    void varDeclaration();
    void funDeclaration();
    void processDeclaration();
    void expressionStatement();
    void printStatement();
    void ifStatement();
    void whileStatement();
    void doWhileStatement();
    void loopStatement();
    void switchStatement();
    void forStatement();
    void returnStatement();
    void block();
    void yieldStatement();
    void fiberStatement();

    int getPrivateIndex(const char *name);
    void handle_assignment(uint8 getOp, uint8 setOp, int arg, bool canAssign);

    void prefixIncrement(bool canAssign);
    void prefixDecrement(bool canAssign);

    // Variables
    uint8 identifierConstant(Token &name);
    void namedVariable(Token &name, bool canAssign);
    void defineVariable(uint8 global);
    void declareVariable();
    void addLocal(Token &name);
    int resolveLocal(Token &name);
    void markInitialized();

    uint8 argumentList();

    void compileFunction(Function* func, bool isProcess);
    void compileProcess(const std::string &name);

    bool isProcessFunction(const char *name) const;

    // Scope
    void beginScope();
    void endScope();

    bool inProcessFunction() const;

    void initRules();

    void frameStatement();
    void exitStatement();

    static ParseRule rules[TOKEN_COUNT];
};