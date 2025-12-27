
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include "interpreter.hpp"
#include "pool.hpp"
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
#include <type_traits>

int totalTests = 0;
int passedTests = 0;
int failedTests = 0;

template <typename T>
class ArrayTest
{
private:
    T *data_;
    size_t size_;
    size_t capacity_;

    void resize(size_t newCapacity)
    {
        if (newCapacity <= capacity_)
            return;

        T *newData = new T[newCapacity];

        // Copy-construct cada elemento
        for (size_t i = 0; i < size_; i++)
        {
            new (&newData[i]) T(std::move(data_[i])); // Placement new
        }

        // Desaloca SEM chamar destructores (já foram moved)
        ::operator delete[](data_); // Raw dealloc

        data_ = newData;
        capacity_ = newCapacity;
    }

public:
    ArrayTest() : data_(nullptr), size_(0), capacity_(0) {}

    ~ArrayTest()
    {
        clear();
        if (data_)
            ::operator delete[](data_);
    }

    void push(const T &value)
    {
        if (size_ >= capacity_)
            resize(capacity_ < 8 ? 8 : capacity_ * 2);

        new (&data_[size_++]) T(value); // Placement new
    }

    void pop()
    {
        if (size_ > 0)
        {
            data_[--size_].~T(); // Explicit destructor
        }
    }

    void clear()
    {
        for (size_t i = 0; i < size_; i++)
            data_[i].~T();
        size_ = 0;
    }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    T &operator[](size_t i) { return data_[i]; }
    const T &operator[](size_t i) const { return data_[i]; }
};

#define TEST(name)                                                 \
    void test_##name();                                            \
    struct TestRegistrar_##name                                    \
    {                                                              \
        TestRegistrar_##name()                                     \
        {                                                          \
            std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"; \
            std::cout << "🧪 TEST: " << #name << "\n";             \
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";   \
            totalTests++;                                          \
            try                                                    \
            {                                                      \
                test_##name();                                     \
                passedTests++;                                     \
                std::cout << "✅ PASSED\n";                        \
            }                                                      \
            catch (const std::exception &e)                        \
            {                                                      \
                failedTests++;                                     \
                std::cout << "❌ FAILED: " << e.what() << "\n";    \
            }                                                      \
        }                                                          \
    } testRegistrar_##name;                                        \
    void test_##name()

// Helper for converting values to string
template <typename T>
std::string valueToString(const T &val)
{
    if constexpr (std::is_same_v<T, std::string>)
    {
        return "\"" + val + "\"";
    }
    else if constexpr (std::is_same_v<T, const char *> || std::is_same_v<T, char *>)
    {
        return std::string("\"") + val + "\"";
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        return val ? "true" : "false";
    }
    else if constexpr (std::is_arithmetic_v<T>)
    {
        return std::to_string(val);
    }
    else
    {
        return "<unknown>";
    }
}

#define ASSERT_EQ(a, b)                                                \
    do                                                                 \
    {                                                                  \
        auto val_a = (a);                                              \
        auto val_b = (b);                                              \
        if (val_a != val_b)                                            \
        {                                                              \
            throw std::runtime_error(                                  \
                std::string("Assertion failed: ") + #a + " == " + #b + \
                "\n  Expected: " + valueToString(val_b) +              \
                "\n  Got:      " + valueToString(val_a));              \
        }                                                              \
    } while (0)

#define ASSERT_DOUBLE_EQ(a, b)                                        \
    do                                                                \
    {                                                                 \
        double val_a = (a);                                           \
        double val_b = (b);                                           \
        if (std::abs(val_a - val_b) > 0.0001)                         \
        {                                                             \
            throw std::runtime_error(                                 \
                std::string("Assertion failed: ") + #a + " ≈ " + #b + \
                "\n  Expected: " + std::to_string(val_b) +            \
                "\n  Got:      " + std::to_string(val_a));            \
        }                                                             \
    } while (0)

#define ASSERT_TRUE(cond)                                                \
    do                                                                   \
    {                                                                    \
        if (!(cond))                                                     \
        {                                                                \
            throw std::runtime_error(                                    \
                std::string("Assertion failed: ") + #cond + " is true"); \
        }                                                                \
    } while (0)

#define ASSERT_FALSE(cond)                                                \
    do                                                                    \
    {                                                                     \
        if (cond)                                                         \
        {                                                                 \
            throw std::runtime_error(                                     \
                std::string("Assertion failed: ") + #cond + " is false"); \
        }                                                                 \
    } while (0)

#define ASSERT_NEAR(actual, expected, epsilon)                                       \
    do                                                                               \
    {                                                                                \
        if (std::abs((actual) - (expected)) > (epsilon))                             \
        {                                                                            \
            printf("❌ FAILED: Assertion failed: %s near %s\n", #actual, #expected); \
            printf("  Expected: %.10f\n", (double)(expected));                       \
            printf("  Got:      %.10f\n", (double)(actual));                         \
            printf("  Diff:     %.10f (max allowed: %.10f)\n",                       \
                   std::abs((actual) - (expected)), (double)(epsilon));              \
            return;                                                                  \
        }                                                                            \
    } while (0)

#define ASSERT_STREQ(a, b)                                             \
    do                                                                 \
    {                                                                  \
        const char *val_a = (a);                                       \
        const char *val_b = (b);                                       \
        if (std::strcmp(val_a, val_b) != 0)                            \
        {                                                              \
            throw std::runtime_error(                                  \
                std::string("Assertion failed: ") + #a + " == " + #b + \
                "\n  Expected: \"" + std::string(val_b) + "\"" +       \
                "\n  Got:      \"" + std::string(val_a) + "\"");       \
        }                                                              \
    } while (0)

#define ASSERT_STRNE(a, b)                                             \
    do                                                                 \
    {                                                                  \
        const char *val_a = (a);                                       \
        const char *val_b = (b);                                       \
        if (std::strcmp(val_a, val_b) == 0)                            \
        {                                                              \
            throw std::runtime_error(                                  \
                std::string("Assertion failed: ") + #a + " != " + #b + \
                "\n  Both are: \"" + std::string(val_a) + "\"");       \
        }                                                              \
    } while (0)


#define ASSERT_NE(a, b)                                                \
    do                                                                 \
    {                                                                  \
        auto val_a = (a);                                              \
        auto val_b = (b);                                              \
        if (val_a == val_b)                                            \
        {                                                              \
            throw std::runtime_error(                                  \
                std::string("Assertion failed: ") + #a + " != " + #b + \
                "\n  Expected different values" +                      \
                "\n  Both are: " + valueToString(val_a));              \
        }                                                              \
    } while (0)    
 
#define ASSERT_LT(a, b)                                               \
    do                                                                \
    {                                                                 \
        auto val_a = (a);                                             \
        auto val_b = (b);                                             \
        if (!(val_a < val_b))                                         \
        {                                                             \
            throw std::runtime_error(                                 \
                std::string("Assertion failed: ") + #a + " < " + #b + \
                "\n  Expected: " + valueToString(val_b) +             \
                "\n  Got:      " + valueToString(val_a));             \
        }                                                             \
    } while (0)

#define ASSERT_LE(a, b)                                                \
    do                                                                 \
    {                                                                  \
        auto val_a = (a);                                              \
        auto val_b = (b);                                              \
        if (!(val_a <= val_b))                                         \
        {                                                              \
            throw std::runtime_error(                                  \
                std::string("Assertion failed: ") + #a + " <= " + #b + \
                "\n  Expected: " + valueToString(val_b) +              \
                "\n  Got:      " + valueToString(val_a));              \
        }                                                              \
    } while (0)

#define ASSERT_GT(a, b)                                               \
    do                                                                \
    {                                                                 \
        auto val_a = (a);                                             \
        auto val_b = (b);                                             \
        if (!(val_a > val_b))                                         \
        {                                                             \
            throw std::runtime_error(                                 \
                std::string("Assertion failed: ") + #a + " > " + #b + \
                "\n  Expected: " + valueToString(val_b) +             \
                "\n  Got:      " + valueToString(val_a));             \
        }                                                             \
    } while (0)

#define ASSERT_GE(a, b)                                                \
    do                                                                 \
    {                                                                  \
        auto val_a = (a);                                              \
        auto val_b = (b);                                              \
        if (!(val_a >= val_b))                                         \
        {                                                              \
            throw std::runtime_error(                                  \
                std::string("Assertion failed: ") + #a + " >= " + #b + \
                "\n  Expected: " + valueToString(val_b) +              \
                "\n  Got:      " + valueToString(val_a));              \
        }                                                              \
    } while (0)

#define ASSERT_DOUBLE_EQ(a, b)                                        \
    do                                                                \
    {                                                                 \
        double val_a = (a);                                           \
        double val_b = (b);                                           \
        if (std::abs(val_a - val_b) > 0.0001)                         \
        {                                                             \
            throw std::runtime_error(                                 \
                std::string("Assertion failed: ") + #a + " ≈ " + #b + \
                "\n  Expected: " + std::to_string(val_b) +            \
                "\n  Got:      " + std::to_string(val_a));            \
        }                                                             \
    } while (0)

#define ASSERT_TRUE(cond)                                                \
    do                                                                   \
    {                                                                    \
        if (!(cond))                                                     \
        {                                                                \
            throw std::runtime_error(                                    \
                std::string("Assertion failed: ") + #cond + " is true"); \
        }                                                                \
    } while (0)

#define ASSERT_FALSE(cond)                                                \
    do                                                                    \
    {                                                                     \
        if (cond)                                                         \
        {                                                                 \
            throw std::runtime_error(                                     \
                std::string("Assertion failed: ") + #cond + " is false"); \
        }                                                                 \
    } while (0)

#define ASSERT_NEAR(actual, expected, epsilon)                                       \
    do                                                                               \
    {                                                                                \
        if (std::abs((actual) - (expected)) > (epsilon))                             \
        {                                                                            \
            printf("❌ FAILED: Assertion failed: %s near %s\n", #actual, #expected); \
            printf("  Expected: %.10f\n", (double)(expected));                       \
            printf("  Got:      %.10f\n", (double)(actual));                         \
            printf("  Diff:     %.10f (max allowed: %.10f)\n",                       \
                   std::abs((actual) - (expected)), (double)(epsilon));              \
            return;                                                                  \
        }                                                                            \
    } while (0)

#define ASSERT_NULL(ptr)                                                \
    do                                                                  \
    {                                                                   \
        if ((ptr) != nullptr)                                           \
        {                                                               \
            throw std::runtime_error(                                   \
                std::string("Assertion failed: ") + #ptr + " is NULL"); \
        }                                                               \
    } while (0)

#define ASSERT_NOT_NULL(ptr)                                                \
    do                                                                      \
    {                                                                       \
        if ((ptr) == nullptr)                                               \
        {                                                                   \
            throw std::runtime_error(                                       \
                std::string("Assertion failed: ") + #ptr + " is NOT NULL"); \
        }                                                                   \
    } while (0)



TEST(StringPoolCache)
{
    StringPool::instance().clear();

    String *s1 = StringPool::instance().create("test");
    ASSERT_EQ(s1->refCount, 1);

    String *s2 = StringPool::instance().create("test");
    ASSERT_EQ(s1, s2);          // Mesmo pointer
    ASSERT_EQ(s2->refCount, 1); // refCount NÃO muda
}

TEST(StringPoolWithValue)
{
    StringPool::instance().clear();

    String *s = StringPool::instance().create("test");
    ASSERT_EQ(s->refCount, 1);

    Value v = Value::makeString(s);
    ASSERT_EQ(s->refCount, 2); // grab() pelo Value
}

TEST(StringPoolValueCleanup)
{
    StringPool::instance().clear();

    String *s = StringPool::instance().create("test");
    ASSERT_EQ(s->refCount, 1);

    {
        Value v = Value::makeString(s);
        ASSERT_EQ(s->refCount, 2); // grab()
    }

    // v saiu de scope aqui
    ASSERT_EQ(s->refCount, 1); // release() chamado
}
TEST(StringPoolClearDeletesRefCount1)
{
    StringPool::instance().clear();

    String *s = StringPool::instance().create("test");
    ASSERT_EQ(s->refCount, 1);

    // Pool tem 1 string
    ASSERT_EQ(StringPool::instance().map.size(), 1);

    StringPool::instance().clear();

    // String foi deletada
    ASSERT_EQ(StringPool::instance().map.size(), 0);
}

TEST(StringPoolEmptyString)
{
    StringPool::instance().clear();

    String *s1 = StringPool::instance().create("");
    String *s2 = StringPool::instance().create("");

    ASSERT_EQ(s1, s2); // Empty strings são cached
    ASSERT_EQ(s1->length(), 0);
    ASSERT_EQ(s1->refCount, 1);
}

TEST(StringPoolSmallString)
{
    StringPool::instance().clear();

    String *s = StringPool::instance().create("small");

    ASSERT_FALSE(s->isLong()); // < 23 chars = small
    ASSERT_EQ(s->length(), 5);
    ASSERT_STREQ(s->chars(), "small");
}

TEST(StringPoolLongString)
{
    StringPool::instance().clear();

    // String com 30 chars (> 23)
    String *s = StringPool::instance().create("this_is_a_very_long_string_test");

    ASSERT_TRUE(s->isLong()); // > 23 chars = long
    ASSERT_EQ(s->length(), 31);
    ASSERT_STREQ(s->chars(), "this_is_a_very_long_string_test");
}

TEST(StringRefCountIncreases)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("test");
    int oldCount = s->refCount;
    
    s->grab();
    
    ASSERT_GT(s->refCount, oldCount);
}

TEST(StringPoolDifferentPointers)
{
    StringPool::instance().clear();
    
    String *s1 = StringPool::instance().create("foo");
    String *s2 = StringPool::instance().create("bar");
    
    ASSERT_NE(s1, s2);
}


TEST(StringUpperBasic)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("hello");
    String *upper = StringPool::instance().upper(s);
    
    ASSERT_STREQ(upper->chars(), "HELLO");
}

TEST(StringContains)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("Hello World");
    String *sub = StringPool::instance().create("World");
    
    ASSERT_TRUE(StringPool::instance().contains(s, sub));
}
 
TEST(StringPoolThresholdBoundary)
{
    StringPool::instance().clear();
    
    // Exatamente 23 chars (threshold)
    String *s23 = StringPool::instance().create("12345678901234567890123");
    ASSERT_FALSE(s23->isLong());
    
    // 24 chars (threshold + 1)
    String *s24 = StringPool::instance().create("123456789012345678901234");
    ASSERT_TRUE(s24->isLong());
}

// ========================================
// INTERNING / CACHE
// ========================================

TEST(StringPoolInterningSame)
{
    StringPool::instance().clear();
    
    String *s1 = StringPool::instance().create("interned");
    String *s2 = StringPool::instance().create("interned");
    String *s3 = StringPool::instance().create("interned");
    
    // Todos apontam para mesma string
    ASSERT_EQ(s1, s2);
    ASSERT_EQ(s2, s3);
    
    // RefCount não muda (pool não incrementa)
    ASSERT_EQ(s1->refCount, 1);
}

TEST(StringPoolInterningDifferent)
{
    StringPool::instance().clear();
    
    String *s1 = StringPool::instance().create("foo");
    String *s2 = StringPool::instance().create("bar");
    
    // Diferentes strings
    ASSERT_NE(s1, s2);
    ASSERT_STREQ(s1->chars(), "foo");
    ASSERT_STREQ(s2->chars(), "bar");
}

TEST(StringPoolCaseSensitive)
{
    StringPool::instance().clear();
    
    String *s1 = StringPool::instance().create("Test");
    String *s2 = StringPool::instance().create("test");
    
    // Case-sensitive
    ASSERT_NE(s1, s2);
}

// ========================================
// REFERENCE COUNTING
// ========================================

TEST(StringRefCountGrab)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("test");
    ASSERT_EQ(s->refCount, 1);
    
    s->grab();
    ASSERT_EQ(s->refCount, 2);
    
    s->grab();
    ASSERT_EQ(s->refCount, 3);
}

TEST(StringRefCountRelease)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("test");
    s->grab();
    s->grab();
    ASSERT_EQ(s->refCount, 3);
    
    s->release();
    ASSERT_EQ(s->refCount, 2);
    
    s->release();
    ASSERT_EQ(s->refCount, 1);
    
    // NÃO libera ainda (pool tem referência)
}

TEST(StringRefCountZeroDeletes)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("temp");
    int poolSize = StringPool::instance().map.size();
    
    s->grab();
    s->release();  // Back to 1
    
    // String ainda existe (pool tem ref)
    ASSERT_EQ(StringPool::instance().map.size(), poolSize);
}

// ========================================
// VALUE INTEGRATION
// ========================================

TEST(ValueGrabsOnAssignment)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("value");
    ASSERT_EQ(s->refCount, 1);
    
    Value v1 = Value::makeString(s);
    ASSERT_EQ(s->refCount, 2);
    
    Value v2 = v1;  // Copy constructor
    ASSERT_EQ(s->refCount, 3);
}

TEST(ValueReleasesOnDestruction)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("value");
    
    {
        Value v1 = Value::makeString(s);
        {
            Value v2 = Value::makeString(s);
            ASSERT_EQ(s->refCount, 3);
        }
        ASSERT_EQ(s->refCount, 2);  // v2 released
    }
    ASSERT_EQ(s->refCount, 1);  // v1 released
}

TEST(ValueAssignmentReleasesOld)
{
    StringPool::instance().clear();
    
    String *s1 = StringPool::instance().create("first");
    String *s2 = StringPool::instance().create("second");
    
    Value v = Value::makeString(s1);
    ASSERT_EQ(s1->refCount, 2);
    ASSERT_EQ(s2->refCount, 1);
    
    v = Value::makeString(s2);  // Reassign
    
    ASSERT_EQ(s1->refCount, 1);  // Released
    ASSERT_EQ(s2->refCount, 2);  // Grabbed
}

// ========================================
// CONCATENATION
// ========================================

TEST(StringConcatBasic)
{
    StringPool::instance().clear();
    
    String *s1 = StringPool::instance().create("Hello");
    String *s2 = StringPool::instance().create("World");
    String *result = StringPool::instance().concat(s1, s2);
    
    ASSERT_STREQ(result->chars(), "HelloWorld");
    ASSERT_EQ(result->length(), 10);
}

TEST(StringConcatEmpty)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("test");
    String *empty = StringPool::instance().create("");
    
    String *r1 = StringPool::instance().concat(s, empty);
    String *r2 = StringPool::instance().concat(empty, s);
    
    // Retorna original (optimization)
    ASSERT_EQ(r1, s);
    ASSERT_EQ(r2, s);
}

TEST(StringConcatSmallToLong)
{
    StringPool::instance().clear();
    
    String *s1 = StringPool::instance().create("short");
    String *s2 = StringPool::instance().create("this_will_make_it_very_long");
    String *result = StringPool::instance().concat(s1, s2);
    
    ASSERT_TRUE(result->isLong());
    ASSERT_STREQ(result->chars(), "shortthis_will_make_it_very_long");
}

// ========================================
// TRANSFORMATIONS
// ========================================

 

TEST(StringLowerBasic)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("WORLD");
    String *lower = StringPool::instance().lower(s);
    
    ASSERT_STREQ(lower->chars(), "world");
}

TEST(StringTrim)
{
    StringPool::instance().clear();
    
    String *s1 = StringPool::instance().create("  test  ");
    String *trimmed = StringPool::instance().trim(s1);
    
    ASSERT_STREQ(trimmed->chars(), "test");
    ASSERT_EQ(trimmed->length(), 4);
}

TEST(StringTrimEmpty)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("   ");
    String *trimmed = StringPool::instance().trim(s);
    
    ASSERT_EQ(trimmed->length(), 0);
}

// ========================================
// SUBSTRING
// ========================================

TEST(StringSubstringBasic)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("Hello World");
    String *sub = StringPool::instance().substring(s, 0, 5);
    
    ASSERT_STREQ(sub->chars(), "Hello");
}

TEST(StringSubstringMiddle)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("0123456789");
    String *sub = StringPool::instance().substring(s, 3, 7);
    
    ASSERT_STREQ(sub->chars(), "3456");
}

TEST(StringSubstringOutOfBounds)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("test");
    String *sub = StringPool::instance().substring(s, 0, 100);
    
    // Clamp to length
    ASSERT_STREQ(sub->chars(), "test");
}

// ========================================
// SEARCH
// ========================================

 

TEST(StringStartsWith)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("Hello World");
    String *prefix = StringPool::instance().create("Hello");
    String *wrong = StringPool::instance().create("World");
    
    ASSERT_TRUE(StringPool::instance().startsWith(s, prefix));
    ASSERT_FALSE(StringPool::instance().startsWith(s, wrong));
}

TEST(StringEndsWith)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("Hello World");
    String *suffix = StringPool::instance().create("World");
    String *wrong = StringPool::instance().create("Hello");
    
    ASSERT_TRUE(StringPool::instance().endsWith(s, suffix));
    ASSERT_FALSE(StringPool::instance().endsWith(s, wrong));
}

TEST(StringIndexOf)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("Hello World Hello");
    String *sub = StringPool::instance().create("Hello");
    
    // Primeira ocorrência
    int idx = StringPool::instance().indexOf(s, sub, 0);
    ASSERT_EQ(idx, 0);
    
    // Segunda ocorrência
    idx = StringPool::instance().indexOf(s, sub, 1);
    ASSERT_EQ(idx, 12);
    
    // Não encontrado
    String *missing = StringPool::instance().create("xyz");
    idx = StringPool::instance().indexOf(s, missing, 0);
    ASSERT_EQ(idx, -1);
}

// ========================================
// EDGE CASES
// ========================================

TEST(StringNullTermination)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("test");
    
    // Verifica null terminator
    ASSERT_EQ(s->chars()[s->length()], '\0');
}

TEST(StringHashConsistency)
{
    StringPool::instance().clear();
    
    String *s1 = StringPool::instance().create("test");
    String *s2 = StringPool::instance().create("test");
    
    // Mesma string = mesmo hash
    ASSERT_EQ(s1->hash, s2->hash);
}

TEST(StringRepeat)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("ab");
    String *repeated = StringPool::instance().repeat(s, 3);
    
    ASSERT_STREQ(repeated->chars(), "ababab");
}

TEST(StringRepeatZero)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("test");
    String *repeated = StringPool::instance().repeat(s, 0);
    
    ASSERT_EQ(repeated->length(), 0);
}

// ========================================
// MEMORY CLEANUP
// ========================================

TEST(StringPoolClearReleasesAll)
{
    StringPool::instance().clear();
    
    String *s1 = StringPool::instance().create("one");
    String *s2 = StringPool::instance().create("two");
    String *s3 = StringPool::instance().create("three");
    
    ASSERT_EQ(StringPool::instance().map.size(), 3);
    
    StringPool::instance().clear();
    
    ASSERT_EQ(StringPool::instance().map.size(), 0);
}

TEST(StringPoolDoesNotDeleteActive)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("active");
    s->grab();  // RefCount = 2
    
    // Clear não deleta (refCount > 1)
    StringPool::instance().clear();
    
    ASSERT_EQ(s->refCount, 2);  // Ainda viva
    
    s->release();  // Cleanup manual
}

// ========================================
// TESTES COM ASSERT_NE
// ========================================

 
TEST(StringPoolNotNull)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("test");
    
    ASSERT_NOT_NULL(s);
    ASSERT_NOT_NULL(s->chars());
}

// ========================================
// TESTES COM ASSERT_STRNE
// ========================================

TEST(StringTransformChanges)
{
    StringPool::instance().clear();
    
    String *original = StringPool::instance().create("test");
    String *upper = StringPool::instance().upper(original);
    
    ASSERT_STRNE(original->chars(), upper->chars());
    ASSERT_STREQ(upper->chars(), "TEST");
}

// ========================================
// TESTES COM ASSERT_GT/LT
// ========================================

 

TEST(StringRefCountDecreases)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("test");
    s->grab();
    s->grab();
    
    int oldCount = s->refCount;
    s->release();
    
    ASSERT_LT(s->refCount, oldCount);
    ASSERT_EQ(s->refCount, oldCount - 1);
}

TEST(StringLengthGrows)
{
    StringPool::instance().clear();
    
    String *s1 = StringPool::instance().create("ab");
    String *s2 = StringPool::instance().create("cde");
    String *concat = StringPool::instance().concat(s1, s2);
    
    ASSERT_GT(concat->length(), s1->length());
    ASSERT_GT(concat->length(), s2->length());
    ASSERT_EQ(concat->length(), s1->length() + s2->length());
}

// ========================================
// TESTES COM ASSERT_LE/GE
// ========================================

TEST(StringIndexBounds)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("Hello");
    String *sub = StringPool::instance().create("lo");
    
    int idx = StringPool::instance().indexOf(s, sub, 0);
    
    ASSERT_GE(idx, 0);  // Encontrou
    ASSERT_LE(idx, (int)s->length() - (int)sub->length());
}

TEST(StringSubstringLength)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("0123456789");
    String *sub = StringPool::instance().substring(s, 2, 7);
    
    ASSERT_LE(sub->length(), s->length());
    ASSERT_EQ(sub->length(), 5);
}

// ========================================
// TESTES DE EDGE CASES
// ========================================

TEST(StringPoolNullSafety)
{
    StringPool::instance().clear();
    
    String *empty = StringPool::instance().create("", 0);
    
    ASSERT_NOT_NULL(empty);
    ASSERT_EQ(empty->length(), 0);
    ASSERT_STREQ(empty->chars(), "");
}

TEST(StringRepeatNegative)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("test");
    String *repeated = StringPool::instance().repeat(s, -1);
    
    // Deve retornar vazia
    ASSERT_EQ(repeated->length(), 0);
}

TEST(StringAtNegativeIndex)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("test");
    String *last = StringPool::instance().at(s, -1);
    
    // Python-style: -1 = último char
    ASSERT_STREQ(last->chars(), "t");
}

TEST(StringAtOutOfBounds)
{
    StringPool::instance().clear();
    
    String *s = StringPool::instance().create("test");
    String *result = StringPool::instance().at(s, 100);
    
    // Retorna vazia
    ASSERT_EQ(result->length(), 0);
}

// ========================================
// STRESS TEST
// ========================================

TEST(StringPoolManyStrings)
{
    StringPool::instance().clear();
    
    const int COUNT = 1000;
    std::vector<String*> strings;
    
    // Cria 1000 strings
    for (int i = 0; i < COUNT; i++)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "string_%d", i);
        strings.push_back(StringPool::instance().create(buf));
    }
    
    // Verifica que todas existem
    ASSERT_EQ(StringPool::instance().map.size(), COUNT);
    
    // Verifica que são diferentes
    for (int i = 0; i < COUNT; i++)
    {
        for (int j = i + 1; j < COUNT; j++)
        {
            ASSERT_NE(strings[i], strings[j]);
        }
    }
}

TEST(StringPoolManyInterned)
{
    StringPool::instance().clear();
    
    // Cria mesma string 1000 vezes
    String *first = nullptr;
    for (int i = 0; i < 1000; i++)
    {
        String *s = StringPool::instance().create("interned");
        
        if (i == 0)
            first = s;
        else
            ASSERT_EQ(s, first);  // Sempre mesmo pointer
    }
    
    // Pool só tem 1 string
    ASSERT_EQ(StringPool::instance().map.size(), 1);
}

TEST(StringPool_SSO_Boundary_23_24)
{
    StringPool::instance().clear();

    std::string s23(23, 'a');
    std::string s24(24, 'b');

    String* a = StringPool::instance().create(s23.c_str(), (uint32)s23.size());
    String* b = StringPool::instance().create(s24.c_str(), (uint32)s24.size());

    ASSERT_EQ(a->length(), 23u);
    ASSERT_EQ(b->length(), 24u);

    ASSERT_TRUE(!a->isLong());   // 23 => small
    ASSERT_TRUE(b->isLong());    // 24 => long

    ASSERT_EQ(std::string(a->chars()), s23);
    ASSERT_EQ(std::string(b->chars()), s24);
}
TEST(StringPool_CacheHit_DifferentBuffersSameText)
{
    StringPool::instance().clear();

    char buf1[] = "hello";
    char buf2[] = "hello";

    String* s1 = StringPool::instance().create(buf1);
    String* s2 = StringPool::instance().create(buf2);

    ASSERT_EQ(s1, s2);
    ASSERT_EQ(s1->refCount, 1);
}

TEST(StringPool_CacheMiss)
{
    StringPool::instance().clear();

    String* a = StringPool::instance().create("hello");
    String* b = StringPool::instance().create("hellO");

    ASSERT_NE(a, b);
}


TEST(Value_CopyAndAssign_AdjustsRefCount)
{
    StringPool::instance().clear();
    String* s = StringPool::instance().create("test");
    ASSERT_EQ(s->refCount, 1);

    Value v1 = Value::makeString(s);
    ASSERT_EQ(s->refCount, 2);

    Value v2 = v1;                // copy ctor
    ASSERT_EQ(s->refCount, 3);

    Value v3;
    v3 = v1;                      // copy assign
    ASSERT_EQ(s->refCount, 4);
}
TEST(VectorOfValues_ReleasesStrings)
{
    StringPool::instance().clear();
    String* s = StringPool::instance().create("test");
    ASSERT_EQ(s->refCount, 1);

    {
        std::vector<Value> vec;
        vec.push_back(Value::makeString(s)); // +1
        vec.push_back(Value::makeString(s)); // +1
        ASSERT_EQ(s->refCount, 3);
    }
    // vec morreu => solta 2 refs
    ASSERT_EQ(s->refCount, 1);
}

TEST(StringPool_Clear_DoesNotDeleteInUse)
{
    StringPool::instance().clear();
    String* s = StringPool::instance().create("hold");
    ASSERT_EQ(s->refCount, 1);

    Value v = Value::makeString(s);
    ASSERT_EQ(s->refCount, 2);

    StringPool::instance().clear(); // deve avisar, mas não destruir
    ASSERT_EQ(s->refCount, 2);

    // quando v morrer, volta a 1 (pool owner)
}


// Call no main:
int main()
{
    std::cout << "\n";

    // Tests run automatically via static constructors

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════╗\n";
    std::cout << "║            TEST SUMMARY                ║\n";
    std::cout << "╚════════════════════════════════════════╝\n";
    std::cout << "Total:  " << totalTests << "\n";
    std::cout << "✅ Pass: " << passedTests << "\n";
    if (failedTests != 0)
        std::cout << "❌ Fail: " << failedTests << "\n";

    if (failedTests == 0)
    {
        std::cout << "\n🎉 ALL  TESTS PASSED! 🎉\n";
        return 0;
    }
    else
    {
        std::cout << "\n💥 SOME TESTS FAILED 💥\n\n";
        return 1;
    }
}