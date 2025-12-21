#pragma once
#include "config.hpp"
#include "string.hpp"
#include "arena.hpp"

 
class StringPool
{
private:
    BlockAllocator allocator;
 
public:
    StringPool() = default;
    ~StringPool() = default;

 
    String *create(const char *str, uint32 len);
   

    String *create(const char *str)
    {
        return create(str, std::strlen(str));
    }

    

    String *concat(String*a, String*b);

 
    void destroy(String *s);
 
 
    void clear()
    {
        allocator.Clear();
    }
 
    static StringPool &instance()
    {
        static StringPool pool;
        return pool;
    }
};


inline String *concatString(String *a , String *b)
{
    return StringPool::instance().concat(a,b);
}
 
inline String *createString(const char *str, uint32 len)
{
    return StringPool::instance().create(str, len);
}

inline String *createString(const char *str)
{
    return StringPool::instance().create(str);
}

inline void destroyString(String *s)
{
    StringPool::instance().destroy(s);
}