#pragma once
#include "config.hpp"
#include "string.hpp"
#include "vector.hpp"
#include "map.hpp"  
#include "types.hpp"  

struct Value;
struct Process;

 
 
 struct CStringHash
{
    size_t operator()(const char *str) const
    {
        // FNV-1a hash
        size_t hash = 2166136261u;
        while (*str)
        {
            hash ^= (unsigned char)*str++;
            hash *= 16777619u;
        }
        return hash;
    }
};


struct CStringEq
{
    bool operator()(const char *a, const char *b) const
    {
        return strcmp(a, b) == 0;
    }
};

class StringPool
{
private:
    HeapAllocator allocator;
    //Vector<String *> strings;
    HashMap<const char *, int, CStringHash, CStringEq> pool;
    
    public:
    StringPool() = default;
    ~StringPool();
    
    Vector<String *> map;

    String *allocString();
    void  deallocString (String *s);


    String *create(const char *str, uint32 len);

    String *create(const char *str);

    String *format(const char *fmt, ...);  


    int indexOf(String *str, String *substr, int startIndex = 0);
    int indexOf(String *str, const char *substr, int startIndex = 0);

    String *concat(String *a, String *b);
    String *upper(String *src);
    String *lower(String *src);
    String *substring(String *src, uint32 start, uint32 end);
    String *replace(String *src, const char *oldStr, const char *newStr);

    String *to_string(Value v);

    String *trim(String *str);
    bool contains(String *str, String *substr);
    bool startsWith(String *str, String *prefix);
    bool endsWith(String *str, String *suffix);
    String *at(String *str, int index);
    String *repeat(String *str, int count);

    String *toString(int value);
    String *toString(double value);

    void destroy(String *s);

    void clear();

    HashMap<const char*, String *, CStringHash, CStringEq> interns;

    static StringPool &instance()
    {
        static StringPool pool;
        return pool;
    }
};


class ProcessPool 
{

    Vector<Process*> pool;
public:
    ProcessPool();
    ~ProcessPool() = default;

    static ProcessPool &instance()
    {
        static ProcessPool pool;
        return pool;
    }

    Process* create();
    void free(Process *proc);
    void destory(Process *proc);
    void clear();

};


inline bool compareString(String *a, String *b)
{
    if (a == nullptr || b == nullptr)
        return false;

  //  Info("Compare string %s %s hash %d %d len %d %d", a->chars(), b->chars(), a->hash, b->hash, a->length(), b->length());

    if (a->hash != b->hash)
        return false;
    if (a == b)
        return true;
    if (a->length() != b->length())
        return false;
    return memcmp(a->chars(), b->chars(), a->length()) == 0;
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
    (void)s;
   // s->release();
}