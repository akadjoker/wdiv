#pragma once
#include "arena.hpp"

struct String
{
  static constexpr size_t SMALL_THRESHOLD = 23;
  static constexpr size_t IS_LONG_FLAG = 0x80000000u;

  size_t hash;
  size_t length_and_flag;

  union
  {
    char *ptr;
    char data[24];
  };

  bool isLong() const { return length_and_flag & IS_LONG_FLAG; }
  size_t length() const { return length_and_flag & ~IS_LONG_FLAG; }

  const char *chars() const { return isLong() ? ptr : data; }
  char *chars() { return isLong() ? ptr : data; }


};


// inline size_t_t hashString(const char *s, int len)
// {
//   size_t_t h = 2166136261u;
//   int i = 0;

//   // Processa 4 bytes de cada vez (unrolled)
//   for (; i + 3 < len; i += 4)
//   {
//     h ^= (uint8_t)s[i];
//     h *= 16777619u;
//     h ^= (uint8_t)s[i + 1];
//     h *= 16777619u;
//     h ^= (uint8_t)s[i + 2];
//     h *= 16777619u;
//     h ^= (uint8_t)s[i + 3];
//     h *= 16777619u;
//   }

//   // Resto
//   for (; i < len; i++)
//   {
//     h ^= (uint8_t)s[i];
//     h *= 16777619u;
//   }

//   return h;
// }


// inline size_t_t hashString(const char* s, int len) {
//   size_t_t h = 2166136261u;
//   for (int i = 0; i < len; i++) {
//     h ^= (uint8_t)s[i];
//     h *= 16777619;
//   }
//   return h;
// }

// inline size_t_t hashString(const char* s, int len) {
//     size_t_t h = 2166136261u;
//     const uint8_t* p = (const uint8_t*)s;
//     const uint8_t* end = p + len;

//     while (p != end) {
//         h ^= *p++;
//         h *= 16777619u;
//     }
//     return h;
// }

inline size_t hashString(const char* s, uint32 len) {
    size_t h = 2166136261u;
    const uint8* p = (const uint8*)s;
    const uint8* end = p + len;

    while (p != end) {
        h ^= *p++;
        h *= 16777619u; // troca por 709607, 127, 131, 16777619, etc.
    }
    return h;
}


//static_assert(sizeof(String) == 32, "ObjString must be 32 bytes");