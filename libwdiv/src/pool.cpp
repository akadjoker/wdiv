#include "pool.hpp"

 

String *StringPool::create(const char *str, uint32 len)
{
    uint32 h = hashString(str, len);

    // Aloca objeto (32 bytes)
    String *s = (String *)allocator.Allocate(sizeof(String));
    s->hash = h;

    if (len <= String::SMALL_THRESHOLD)
    {
        // Small - inline
        s->length_and_flag = len;
        std::memcpy(s->data, str, len);
        s->data[len] = '\0';
    }
    else
    {
        // Long - aloca buffer
        s->length_and_flag = len | String::IS_LONG_FLAG;
        s->ptr = (char *)allocator.Allocate(len + 1);
        std::memcpy(s->ptr, str, len);
        s->ptr[len] = '\0';
    }

    return s;
}

String *StringPool::concat(String *a, String *b)
{
    size_t lenA = a->length();
    size_t lenB = b->length();

    // Fast paths
    if (lenA == 0)
        return b; // "" + "abc" = "abc"
    if (lenB == 0)
        return a; // "abc" + "" = "abc"

    size_t len = lenA + lenB;
    String *s = (String *)allocator.Allocate(sizeof(String));

    if (len <= String::SMALL_THRESHOLD)
    {
        s->length_and_flag = static_cast<uint32>(len);
        std::memcpy(s->data, a->chars(), lenA);
        std::memcpy(s->data + lenA, b->chars(), lenB);
        s->data[len] = '\0';
    }
    else
    {
        s->length_and_flag = static_cast<uint32>(len) | String::IS_LONG_FLAG;
        s->ptr = (char *)allocator.Allocate(len + 1);
        std::memcpy(s->ptr, a->chars(), lenA);
        std::memcpy(s->ptr + lenA, b->chars(), lenB);
        s->ptr[len] = '\0';
    }

    s->hash = hashString(s->chars(), static_cast<int>(len));
    return s;
}

void StringPool::destroy(String *s)
{
    if (!s)
        return;

    if (s->isLong())
    {
        allocator.Free(s->ptr, s->length() + 1);
    }

    allocator.Free(s, sizeof(String));
}