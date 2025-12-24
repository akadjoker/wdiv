#pragma once
#include "config.hpp"


typedef const char* (*FileLoaderCallback)(const char* filename, size_t* outSize, void* userdata);



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