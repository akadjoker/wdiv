#include "value.hpp"
#include "interpreter.hpp"
#include "pool.hpp"
#include "instances.hpp"

Value::Value() : type(ValueType::NIL)
{
    as.integer = 0;
}

// Value::~Value()
// {
//     if(type==ValueType::STRUCT)
//     {
//          printValue(*this);
//          printf("Destroy \n");
//     }
// }

Value Value::makeNil()
{
    Value v;
    v.type = ValueType::NIL;
    return v;
}

Value Value::makeBool(bool b)
{
    Value v;
    v.type = ValueType::BOOL;
    v.as.boolean = b;
    return v;
}

Value Value::makeInt(long i)
{
    Value v;
    v.type = ValueType::INT;
    v.as.integer = i;
    return v;
}

Value Value::makeDouble(double d)
{
    Value v;
    v.type = ValueType::DOUBLE;
    v.as.number = d;
    return v;
}

Value Value::makeFloat(float f)
{
    Value v;
    v.type = ValueType::DOUBLE;
    v.as.number = f;
    return v;
}

Value Value::makeString(const char *str)
{
    Value v;
    v.type = ValueType::STRING;
    v.as.string = createString(str);
    return v;
}

Value Value::makeString(String *s)
{
    Value v;
    v.type = ValueType::STRING;
    v.as.string = s;
    return v;
}

Value Value::makeFunction(int idx)
{
    Value v;
    v.type = ValueType::FUNCTION;
    v.as.functionId = idx;
    return v;
}

Value Value::makeNative(int idx)
{
    Value v;
    v.type = ValueType::NATIVE;
    v.as.nativeId = idx;
    return v;
}

Value Value::makeProcess(int idx)
{
    Value v;
    v.type = ValueType::PROCESS;
    v.as.processId = idx;
    return v;
}

Value Value::makeStruct(int idx)
{
    Value v;
    v.type = ValueType::STRUCT;
    v.as.structId = idx;
    return v;
}

Value Value::makeStructInstance(String *s)
{
    Value v;
    v.type = ValueType::STRUCTINSTANCE;
    v.as.sInstance = InstancePool::instance().createStruct(s);
    return v;
}

Value Value::makeMap()
{
    Value v;
    v.type = ValueType::MAP;
    v.as.map = InstancePool::instance().createMap();
    return v;
}

Value Value::makeArray()
{
    Value v;
    v.type = ValueType::ARRAY;
    v.as.array = InstancePool::instance().createArray();
    return v;
}

Value Value::makeProcNative(int idx)
{
    Value v;
    v.type = ValueType::PROC_NATIVES;
    v.as.procNativesId = idx;
    return v;
}

bool Value::isNumber() const
{
    return ((type == ValueType::INT) || (type == ValueType::DOUBLE));
}

bool Value::asBool() const { return as.boolean; }
long Value::asInt() const
{
    return as.integer;
}

double Value::asDouble() const
{
    if (type == ValueType::DOUBLE)
    {
        return as.number;
    }
    else if (type == ValueType::INT)
    {
        return static_cast<double>(as.integer);
    }
    Warning("Wrong type conversion to double");
    return 0;
}
float Value::asFloat() const
{
    if (type == ValueType::DOUBLE)
    {
        return static_cast<float>(as.number);
    }
    else if (type == ValueType::INT)
    {
        return static_cast<float>(as.integer);
    }
    Warning("Wrong type conversion to float");
    return 0;
}
const char *Value::asStringChars() const { return as.string->chars(); }
String *Value::asString() const { return as.string; }

int Value::asFunctionId() const { return as.functionId; }
int Value::asNativeId() const { return as.nativeId; }
int Value::asProcessId() const { return as.processId; }

int Value::asStructId() const
{
    return as.structId;
}

StructInstance *Value::asStructInstance() const
{
    return as.sInstance;
}

ArrayInstance *Value::asArray() const
{
    return as.array;
}

MapInstance *Value::asMap() const
{
    return as.map;
}

long Value::asNumber() const
{
    if (type == ValueType::DOUBLE)
    {
        return static_cast<long>(as.number);
    }
    else if (type == ValueType::INT)
    {
        return static_cast<long>(as.integer);
    }
    Warning("Wrong type conversion to number");
    return 0;
}

void printValueNewLine(const Value &value)
{
    switch (value.type)
    {
    case ValueType::NIL:
        printf("nil\n");
        break;
    case ValueType::BOOL:
        printf("%s\n", value.as.boolean ? "true" : "false");
        break;
    case ValueType::INT:
        printf("%ld\n", value.as.integer);
        break;
    case ValueType::DOUBLE:
        printf("%f\n", value.as.number);
        break;
    case ValueType::STRING:
        printf("%s\n", value.as.string->chars());
        break;
    case ValueType::FUNCTION:
        printf("<function>\n");
        break;
    case ValueType::NATIVE:
        printf("<native>\n");
        break;
    case ValueType::PROCESS:
        printf("<process>\n");
        break;
    case ValueType::STRUCT:
        printf("<struct>\n");
        break;
    case ValueType::STRUCTINSTANCE:
        printf("<struct_instance>\n");
        break;

    default:
        printf("<?>\n)");
        break;
    }
}

void printValue(const Value &value)
{
    switch (value.type)
    {
    case ValueType::NIL:
        printf("nil");
        break;
    case ValueType::BOOL:
        printf("%s", value.as.boolean ? "true" : "false");
        break;
    case ValueType::INT:
        printf("%ld", value.as.integer);
        break;
    case ValueType::DOUBLE:
        printf("%f", value.as.number);
        break;
    case ValueType::STRING:
        printf("%s", value.as.string->chars());
        break;
    case ValueType::FUNCTION:
        printf("<function>");
        break;
    case ValueType::NATIVE:
        printf("<native>");
        break;
    case ValueType::PROCESS:
        printf("<process>");
        break;
    case ValueType::PROC_NATIVES:
        printf("<process_native>");
        break;
    case ValueType::ARRAY:
    {
        ArrayInstance *arr = value.asArray();
        printf("[");
        for (int i = 0; i < arr->values.size(); i++)
        {
            printValue(arr->values[i]);
            if (i < arr->values.size() - 1)
                printf(", ");
        }
        printf("]");
        break;
    }
    case ValueType::MAP:
    {

        MapInstance *map = value.asMap();
        printf("{");

        int i = 0;
        map->table.forEach([&](String *key, Value val)
        {
        if (i > 0) printf(", ");
        printf("%s: ", key->chars());
        printValue(val);
        i++; });

        printf("}");
        break;
    }

    case ValueType::STRUCT:
        printf("<struct>");
        break;
    case ValueType::STRUCTINSTANCE:
    {
        StructInstance *instance = value.as.sInstance;
        printf("struct '%s' [", instance->def->name->chars());
        int index = 0;

        bool first = true;

        instance->def->names.forEach([&](String *key, int fieldIndex)
                                     {
             if (!first) 
            {
                printf(", ");
            }
            first = false;

            printf("%s = ", key->chars());
            printValue(instance->values[fieldIndex]); });

        printf("]\n");
    }
    break;

    default:
        printf("<?>");
        break;
    }
}

void printValueNl(const Value &value)
{
    switch (value.type)
    {
    case ValueType::NIL:
        printf("nil\n");
        break;
    case ValueType::BOOL:
        printf("%s\n", value.as.boolean ? "true" : "false");
        break;
    case ValueType::INT:
        printf("%ld\n", value.as.integer);
        break;
    case ValueType::DOUBLE:
        printf("%f\n", value.as.number);
        break;
    case ValueType::STRING:
        printf("%s\n", value.as.string->chars());
        break;
    case ValueType::FUNCTION:
        printf("<function>\n");
        break;
    case ValueType::NATIVE:
        printf("<native>\n");
        break;
    case ValueType::PROCESS:
        printf("<process>\n");
        break;
    case ValueType::PROC_NATIVES:
        printf("<process_native>\n");
        break;
    case ValueType::ARRAY:
        printf("<array>\n");
        break;
    case ValueType::MAP:
        printf("<map>\n");
        break;
    case ValueType::STRUCT:
        printf("<struct>\n");
        break;
    case ValueType::STRUCTINSTANCE:
        printf("<struct_instance>\n");
        break;

    default:
        printf("<?>");
        break;
    }
}

bool valuesEqual(const Value &a, const Value &b)
{
    if (a.type != b.type)
        return false;

    switch (a.type)
    {
    case ValueType::INT:
        return a.asInt() == b.asInt();
    case ValueType::BOOL:
        return a.asBool() == b.asBool();
    case ValueType::NIL:
        return true;
    case ValueType::STRING:
        return a.asString() == b.asString();
    case ValueType::DOUBLE:
        return a.asDouble() == b.asDouble();
    case ValueType::STRUCT:
    case ValueType::MAP:
    case ValueType::ARRAY:
    case ValueType::FUNCTION:
    case ValueType::PROCESS:
    case ValueType::PROC_NATIVES:
        return a.type == b.type;
    default:
        return false;
    }
}
