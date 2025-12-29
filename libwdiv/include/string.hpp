#pragma once
#include "arena.hpp"
#include "types.hpp"

struct String
{
  static constexpr size_t SMALL_THRESHOLD = 23;
  static constexpr size_t IS_LONG_FLAG = 0x80000000u;

  int index;
  size_t hash;
  size_t length_and_flag;

  union
  {
    char *ptr;
    char data[24];
  };

  bool isLong() const { return length_and_flag & IS_LONG_FLAG; }
  size_t length() const;

  const char *chars() const { return isLong() ? ptr : data; }
  char *chars() { return isLong() ? ptr : data; }
};

 

inline size_t hashString(const char *s, uint32 len)
{
  size_t h = 2166136261u;
  const uint8 *p = (const uint8 *)s;
  const uint8 *end = p + len;

  while (p != end)
  {
    h ^= *p++;
    h *= 16777619u;  
  }
  return h;
}

 