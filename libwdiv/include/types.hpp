#pragma once
#include "config.hpp"

typedef const char *(*FileLoaderCallback)(const char *filename, size_t *outSize, void *userdata);

static constexpr int MAX_PRIVATES = 16;
static constexpr int MAX_FIBERS = 8;
static constexpr int STACK_MAX = 256;
static constexpr int FRAMES_MAX = 32;
static constexpr int GOSUB_MAX = 16;

enum class InterpretResult : uint8
{
    OK,
    COMPILE_ERROR,
    RUNTIME_ERROR
};

enum class FiberState : uint8
{
    RUNNING,
    SUSPENDED,
    DEAD
};

enum class FunctionType : uint8
{
    TYPE_FUNCTION,    // def normal
    TYPE_METHOD,      // método de class
    TYPE_INITIALIZER, // init (construtor)
    TYPE_SCRIPT       // top-level script
};

enum GCObjectType : uint8
{
    GC_NONE,
    GC_STRING,
    GC_CLASSINSTANCE,
    GC_ARRAY,
    GC_MAP,
    GC_STRUCTINSTANCE,
    GC_NATIVECLASSINSTANCE,
    GC_NATIVESTRUCTINSTANCE,
};

struct GCObject
{
    bool marked = false;
    GCObjectType type  = GC_NONE;

    GCObject();
    virtual ~GCObject();
    virtual void drop() {}
};
