
#pragma once
 
#include <cstddef>
#include <cassert>
#include <cfloat>
#include <cstring>
#include <cstdlib>
#include <assert.h>
#include <cstdio>
#include <cmath>

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef float float32;
typedef double float64;

const float32 maxFloat = FLT_MAX;
const float32 epsilon = FLT_EPSILON;
const float32 pi = 3.14159265359f;

template <typename T>
inline T Max(T a, T b)
{
    return a > b ? a : b;
}


#define CONSOLE_COLOR_RESET "\033[0m"
#define CONSOLE_COLOR_GREEN "\033[1;32m"
#define CONSOLE_COLOR_RED "\033[1;31m"
#define CONSOLE_COLOR_PURPLE "\033[1;35m"
#define CONSOLE_COLOR_CYAN "\033[0;36m"
#define CONSOLE_COLOR_YELLOW "\033[1;33m"
#define CONSOLE_COLOR_BLUE "\033[0;34m"


 
void Warning( const char *fmt, ...);
void Info( const char *fmt, ...);
void Error( const char *fmt, ...);
void Print( const char *fmt, ...);
void Trace(int severity,  const char *fmt, ...);

#define INFO(fmt, ...) Log(0, fmt, ##__VA_ARGS__)
#define WARNING(fmt, ...) Log(1, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) Log(2, fmt, ##__VA_ARGS__)
#define PRINT(fmt, ...) Log(3, fmt, ##__VA_ARGS__)

char *LoadTextFile(const char *fileName);
void FreeTextFile(char *text);

void* aAlloc(size_t size);
void* aRealloc(void* buffer,size_t size);
void aFree(void* mem);

const char *longToString(long value);
const char *doubleToString(double value);




#if defined(_DEBUG)
#include <assert.h>
#define DEBUG_BREAK_IF(condition) if (condition) { printf("Debug break: %s at %s:%d\n", #condition, __FILE__, __LINE__); std::exit(EXIT_FAILURE); }
#else
#define DEBUG_BREAK_IF(_CONDITION_)
#endif

inline size_t CalculateCapacityGrow(size_t capacity, size_t minCapacity)
{
    if (capacity < minCapacity)
    capacity = minCapacity;
    if (capacity < 8)
    {
        capacity = 8;
    }
    else
    {
        // Round up to the next power of 2 and multiply by 2 (http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2)
        capacity--;
        capacity |= capacity >> 1;
        capacity |= capacity >> 2;
        capacity |= capacity >> 4;
        capacity |= capacity >> 8;
        capacity |= capacity >> 16;
        capacity++;
    }
    return capacity;
}

static inline size_t  GROW_CAPACITY(size_t capacity)
{
    return ((capacity) < 8 ? 8 : (capacity) * 2);
}