#include "pool.hpp"
#include "value.hpp"
#include "arena.hpp"
#include "interpreter.hpp"
#include <ctype.h>

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


   // Warning(" Create string %s", s->chars());
    return s;
}

void StringPool::destroy(String *s)
{
    if (!s)
        return;

    //Warning(" Destroy string %s", s->chars());
    if (s->isLong())
    {

        allocator.Free(s->ptr, s->length() + 1);
        s->ptr=nullptr;
    }

    allocator.Free(s, sizeof(String));
}

String *StringPool::create(const char *str)
{
        return create(str, std::strlen(str));
}



String *StringPool::toString(int value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    return create(buf, false);
}

String *StringPool::toString(double value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f", value);
    return create(buf, false);
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



void StringPool::clear()
{

   

    //Info("String pool stats:");
 //   allocator.Stats();
 
    allocator.Clear();
}

String *StringPool::upper(String *src)
{
    if (!src)
        return nullptr;

    size_t len = src->length();
    const char *str = src->chars();

    // Aloca objeto String (32 bytes)
    String *s = (String *)allocator.Allocate(sizeof(String));

    if (len <= String::SMALL_THRESHOLD)
    {
        // Small - inline
        s->length_and_flag = static_cast<uint32>(len);

        for (size_t i = 0; i < len; i++)
        {
            s->data[i] = (char)toupper((unsigned char)str[i]);
        }
        s->data[len] = '\0';
    }
    else
    {
        // Long - aloca buffer
        s->length_and_flag = static_cast<uint32>(len) | String::IS_LONG_FLAG;
        s->ptr = (char *)allocator.Allocate(len + 1);

        for (size_t i = 0; i < len; i++)
        {
            s->ptr[i] = (char)toupper((unsigned char)str[i]);
        }
        s->ptr[len] = '\0';
    }

    s->hash = hashString(s->chars(), static_cast<uint32>(len));
    return s;
}

String *StringPool::lower(String *src)
{
    if (!src)
        return nullptr;

    size_t len = src->length();
    const char *str = src->chars();

    // Aloca objeto String (32 bytes)
    String *s = (String *)allocator.Allocate(sizeof(String));

    if (len <= String::SMALL_THRESHOLD)
    {
        // Small - inline
        s->length_and_flag = static_cast<uint32>(len);

        for (size_t i = 0; i < len; i++)
        {
            s->data[i] = (char)tolower((unsigned char)str[i]);
        }
        s->data[len] = '\0';
    }
    else
    {
        // Long - aloca buffer
        s->length_and_flag = static_cast<uint32>(len) | String::IS_LONG_FLAG;
        s->ptr = (char *)allocator.Allocate(len + 1);

        for (size_t i = 0; i < len; i++)
        {
            s->ptr[i] = (char)tolower((unsigned char)str[i]);
        }
        s->ptr[len] = '\0';
    }

    s->hash = hashString(s->chars(), static_cast<uint32>(len));
    return s;
}

String *StringPool::substring(String *src, uint32 start, uint32 end)
{
    if (!src)
        return create("", 0); // String vazia

    size_t len = src->length();

    if (start >= len)
        start = len;
    if (end > len)
        end = len;
    if (start > end)
        start = end;

    size_t newLen = end - start;
    if (newLen == 0)
        return create("", 0); // String vazia

    const char *str = src->chars() + start;
    return create(str, newLen);
}

String *StringPool::replace(String *src, const char *oldStr, const char *newStr)
{
    if (!src || !oldStr || !newStr)
        return src;

    const char *str = src->chars();
    size_t len = src->length();
    size_t oldLen = strlen(oldStr);
    size_t newLen = strlen(newStr);

    if (oldLen == 0)
        return src;

    // Conta ocorrências
    size_t count = 0;
    const char *pos = str;
    while ((pos = strstr(pos, oldStr)) != nullptr)
    {
        count++;
        pos += oldLen;
    }

    if (count == 0)
        return src; // Sem mudanças

    // Calcula tamanho final
    size_t finalLen = len - (count * oldLen) + (count * newLen);

    // Aloca String
    String *s = (String *)allocator.Allocate(sizeof(String));

    char *dest;
    if (finalLen <= String::SMALL_THRESHOLD)
    {
        s->length_and_flag = static_cast<uint32>(finalLen);
        dest = s->data;
    }
    else
    {
        s->length_and_flag = static_cast<uint32>(finalLen) | String::IS_LONG_FLAG;
        s->ptr = (char *)allocator.Allocate(finalLen + 1);
        dest = s->ptr;
    }

    // Copia com substituições
    const char *current = str;
    size_t destIdx = 0;

    while ((pos = strstr(current, oldStr)) != nullptr)
    {
        // Copia até a ocorrência
        size_t copyLen = pos - current;
        std::memcpy(dest + destIdx, current, copyLen);
        destIdx += copyLen;

        // Copia substituto
        std::memcpy(dest + destIdx, newStr, newLen);
        destIdx += newLen;

        current = pos + oldLen;
    }

    // Copia resto
    size_t remainLen = len - (current - str);
    std::memcpy(dest + destIdx, current, remainLen);
    dest[finalLen] = '\0';

    s->hash = hashString(s->chars(), static_cast<uint32>(finalLen));
    return s;
}

String *StringPool::at(String *str, int index)
{
    if (!str)
        return create("", 0);

    int len = str->length();

    if (index < 0)
        index += len;

    if (index < 0 || index >= len)
    {
        return create("", 0); // String vazia
    }

    char buf[2] = {str->chars()[index], '\0'};
    return create(buf, 1);
}

// Contains - verifica se contém substring
bool StringPool::contains(String *str, String *substr)
{
    if (!str || !substr)
        return false;
    if (substr->length() == 0)
        return true; // String vazia sempre contém

    return strstr(str->chars(), substr->chars()) != nullptr;
}

// Trim - remove espaços início/fim
String *StringPool::trim(String *str)
{
    if (!str)
        return create("", 0);

    const char *start = str->chars();
    const char *end = start + str->length() - 1;

    // Trim início
    while (*start && isspace((unsigned char)*start))
        start++;

    // Trim fim
    while (end > start && isspace((unsigned char)*end))
        end--;

    // Se tudo é espaço
    if (end < start)
        return create("", 0);

    size_t len = end - start + 1;
    return create(start, len);
}

bool StringPool::startsWith(String *str, String *prefix)
{
    if (!str || !prefix)
        return false;
    if (prefix->length() > str->length())
        return false;

    return strncmp(str->chars(), prefix->chars(), prefix->length()) == 0;
}

bool StringPool::endsWith(String *str, String *suffix)
{
    if (!str || !suffix)
        return false;

    int strLen = str->length();
    int suffixLen = suffix->length();

    if (suffixLen > strLen)
        return false;

    return strcmp(str->chars() + (strLen - suffixLen),
                  suffix->chars()) == 0;
}

int StringPool::indexOf(String *str, String *substr, int startIndex)
{
    if (!str || !substr)
        return -1;
    if (substr->length() == 0)
        return startIndex;

    int strLen = str->length();
 
    if (startIndex < 0)
        startIndex = 0;
    if (startIndex >= strLen)
        return -1;

 
    const char *start = str->chars() + startIndex;
    const char *found = strstr(start, substr->chars());

    if (!found)
        return -1;

    return (int)(found - str->chars());
}

 
int StringPool::indexOf(String *str, const char *substr, int startIndex)
{
    if (!str || !substr)
        return -1;
    return indexOf(str, create(substr), startIndex);
}

// Repeat - repete string N vezes
String *StringPool::repeat(String *str, int count)
{
    if (!str || count <= 0)
        return create("", 0);
    if (count == 1)
        return str;

    size_t len = str->length();
    size_t totalLen = len * count;

    // Aloca String
    String *s = (String *)allocator.Allocate(sizeof(String));

    char *dest;
    if (totalLen <= String::SMALL_THRESHOLD)
    {
        s->length_and_flag = static_cast<uint32>(totalLen);
        dest = s->data;
    }
    else
    {
        s->length_and_flag = static_cast<uint32>(totalLen) | String::IS_LONG_FLAG;
        s->ptr = (char *)allocator.Allocate(totalLen + 1);
        dest = s->ptr;
    }

    // Repete
    for (int i = 0; i < count; i++)
    {
        std::memcpy(dest + (i * len), str->chars(), len);
    }
    dest[totalLen] = '\0';

    s->hash = hashString(dest, static_cast<uint32>(totalLen));
    return s;
}

//     // Split - divide string por separador
//     Array* split(String* str, String* separator) {
//         Array* result = new Array();
//         result->capacity = 10;
//         result->count = 0;
//         result->elements = new Value[result->capacity];

//         if (!str) return result;

//         const char* strChars = str->chars();
//         int strLen = str->length();

//         // Se separador vazio, split cada char
//         if (!separator || separator->length() == 0) {
//             for (int i = 0; i < strLen; i++) {
//                 char buf[2] = {strChars[i], '\0'};

//                 if (result->count >= result->capacity) {
//                     result->capacity *= 2;
//                     Value* newElements = new Value[result->capacity];
//                     memcpy(newElements, result->elements,
//                            result->count * sizeof(Value));
//                     delete[] result->elements;
//                     result->elements = newElements;
//                 }

//                 result->elements[result->count++] =
//                     Value::makeString(create(buf, 1));
//             }
//             return result;
//         }

//         const char* sepChars = separator->chars();
//         int sepLen = separator->length();

//         int start = 0;
//         for (int i = 0; i <= strLen - sepLen; i++) {
//             if (strncmp(&strChars[i], sepChars, sepLen) == 0) {
//                 // Found separator
//                 int partLen = i - start;

//                 if (result->count >= result->capacity) {
//                     result->capacity *= 2;
//                     Value* newElements = new Value[result->capacity];
//                     memcpy(newElements, result->elements,
//                            result->count * sizeof(Value));
//                     delete[] result->elements;
//                     result->elements = newElements;
//                 }

//                 result->elements[result->count++] =
//                     Value::makeString(create(&strChars[start], partLen));

//                 start = i + sepLen;
//                 i += sepLen - 1;
//             }
//         }

//         // Last part
//         if (start <= strLen) {
//             int partLen = strLen - start;

//             if (result->count >= result->capacity) {
//                 result->capacity *= 2;
//                 Value* newElements = new Value[result->capacity];
//                 memcpy(newElements, result->elements,
//                        result->count * sizeof(Value));
//                 delete[] result->elements;
//                 result->elements = newElements;
//             }

//             result->elements[result->count++] =
//                 Value::makeString(create(&strChars[start], partLen));
//         }

//         return result;
//     }
// };

ProcessPool::ProcessPool()
{
    pool.reserve(512);
}

Process *ProcessPool::create()
{
    Process * proc=nullptr;
    if (!pool.size())
    {
        proc = (Process*) aAlloc(sizeof(Process));
        return proc;
    }

    proc = pool.back();
    pool.pop();
    return proc;
}

void ProcessPool::destory(Process *proc)
{
    pool.push(proc);
}

void ProcessPool::free(Process *proc)
{
        proc->release();
        aFree(proc);
}


void ProcessPool::clear()
{
    for (size_t j = 0; j < pool.size(); j++)
    {
        Process *proc = pool[j];
        proc->release();
        aFree(proc);
    }
    pool.clear();
}
