#pragma once
#include "config.hpp"
#include "string.hpp"
#include "vector.hpp"


struct Value;
struct Process;

 

class StringPool
{
private:
    HeapAllocator allocator;

public:
    StringPool() = default;
    ~StringPool() = default;

    String *create(const char *str, uint32 len);

    String *create(const char *str);



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

    void destroy(String *s);

    void clear();


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
 

inline String *createString(const char *str, uint32 len)
{
    return StringPool::instance().create(str, len);
}

inline String *createString(const char *str)
{
    return StringPool::instance().create(str);
}

inline String *createStaticString(const char *str)
{
    return StringPool::instance().create(str);
}

inline void destroyString(String *s)
{
    StringPool::instance().destroy(s);
}