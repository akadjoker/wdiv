#include "value.hpp"
#include "interpreter.hpp"
#include "pool.hpp"
#include "instances.hpp"



#ifndef NDEBUG
    // Helper interno
    #define VALUE_TYPE_CHECK_IMPL(condition, ...) \
        do { \
            if (!(condition)) { \
                Warning(__VA_ARGS__); \
            } \
        } while(0)
    
    #define VALUE_TYPE_CHECK(condition, ...) \
        VALUE_TYPE_CHECK_IMPL(condition, __VA_ARGS__)
#else
    #define VALUE_TYPE_CHECK(condition, ...) ((void)0)
#endif

 

Value::Value() : type(ValueType::NIL)
{
    as.boolean = false;
}

Value::~Value()
{
    // if (type == ValueType::STRING && as.string)
    //     as.string->release();
    // else if (type == ValueType::STRUCTINSTANCE)
    //     as.sInstance->release();
    // else if (type == ValueType::ARRAY)
    //     as.array->release();
    // else if (type == ValueType::MAP)
    //     as.map->release();
    // else if (type == ValueType::CLASSINSTANCE)
    //     as.sClass->release();
    // else if (type == ValueType::NATIVESTRUCTINSTANCE)
    //     as.sNativeStruct->release();
    // else if (type == ValueType::NATIVECLASSINSTANCE)
    //     as.sClassInstance->release();
}


Value Value::makeString(const char *str)
{
    Value v;
    v.type = ValueType::STRING;
    String* string = createString(str);
    v.as.id = string->index;
    return v;
}

Value Value::makeString(String *s)
{
    Value v;
    v.type = ValueType::STRING;
    v.as.id = s->index;
    return v;
}
Value Value::makeNativeClassInstance()
{
    Value v;
    v.type = ValueType::NATIVECLASSINSTANCE;
    v.as.sClassInstance = InstancePool::instance().createNativeClass();
    return v;
}

Value Value::makeStructInstance()
{
    Value v;
    v.type = ValueType::STRUCTINSTANCE;
    StructInstance * instance = InstancePool::instance().createStruct();
    v.as.id = instance->index;
    return v;
}

Value Value::makeMap()
{
    Value v;
    v.type = ValueType::MAP;
    MapInstance *  map = InstancePool::instance().createMap();
    v.as.id = map->index; 
    return v;
}

Value Value::makeArray()
{
    Value v;
    v.type = ValueType::ARRAY;
    ArrayInstance *   array = InstancePool::instance().createArray();
    v.as.id = array->index;
 
    return v;
}
Value Value::makeNativeStructInstance(uint32 structSize)
{
    Value v;
    v.type = ValueType::NATIVESTRUCTINSTANCE;
    v.as.sNativeStruct = InstancePool::instance().createNativeStruct(structSize);
 
    return v;
}
Value Value::makeClassInstance()
{
    Value v;
    v.type = ValueType::CLASSINSTANCE;
    ClassInstance * instance = InstancePool::instance().createClass();
    v.as.id = instance->index;
 
    return v;
}



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

Value Value::makeByte(uint8 b)
{
    Value v;
    v.type = ValueType::BYTE;
    v.as.byte = b;
    return v;
}

Value Value::makeInt(int i)
{
    Value v;
    v.type = ValueType::INT;
    v.as.integer = i;
    return v;
}

Value Value::makeUInt(uint32 i)
{
    Value v;
    v.type = ValueType::UINT;
    v.as.unsignedInteger = i;
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
    v.type = ValueType::FLOAT;
    v.as.n_float = f;
    return v;
}

Value Value::makeFunction(int idx)
{
    Value v;
    v.type = ValueType::FUNCTION;
    v.as.id = idx;
    return v;
}

Value Value::makeNative(int idx)
{
    Value v;
    v.type = ValueType::NATIVE;
    v.as.id = idx;
    return v;
}

Value Value::makeNativeClass(int idx)
{
    Value v;
    v.type = ValueType::NATIVECLASS;
    v.as.id = idx;
    return v;
}

Value Value::makeProcess(int idx)
{
    Value v;
    v.type = ValueType::PROCESS;
    v.as.id = idx;
    return v;
}

Value Value::makeStruct(int idx)
{
    Value v;
    v.type = ValueType::STRUCT;
    v.as.id = idx;
    return v;
}

Value Value::makeClass(int idx)
{
    Value v;
    v.type = ValueType::CLASSID;
    v.as.id = idx;
    return v;
}

Value Value::makePointer(void *pointer)
{
    Value v;
    v.type = ValueType::POINTER;
    v.as.pointer = pointer;
    return v;
}

Value Value::makeNativeStruct(int idx)
{
    Value v;
    v.type = ValueType::NATIVESTRUCT;
    v.as.id = idx;
    return v;
}

void Value::drop()
{

    
    // if (type == ValueType::STRUCTINSTANCE)
    //  {
    //     Info("drop value %s",  typeToString(type));
    //      as.sInstance->release();
    //     }
    //     else if (type == ValueType::ARRAY)
    //  {
    //      as.array->release();
    //  }
    //  else if (type == ValueType::MAP)
    //  {
    //      as.map->release();
    //  }
    //  else if (type == ValueType::CLASSINSTANCE)
    //  {
    //      Info("drop value %s",  typeToString(type));
    //      as.sClass->release();
    //  }
    //  else    if (type == ValueType::NATIVESTRUCTINSTANCE)
    //  {
    //     Info("drop value %s",  typeToString(type));
    //      as.sNativeStruct->release();
    //  }
    //  else if (type == ValueType::NATIVECLASSINSTANCE)
    //  {
    //     Info("drop value %s",  typeToString(type));
    //      as.sClassInstance->release();
    //  }
    //  else if (type == ValueType::STRING)
    // {
    //     Info("drop value %s",  typeToString(type));
    //     as.string->release();
    //     // Info("Drop value %s %d %s", typeToString(type), as.string->refCount, as.string->chars());
    // }
}

bool Value::isNumber() const
{
    return ((type == ValueType::INT) || (type == ValueType::DOUBLE) || (type == ValueType::BYTE));
}

bool Value::asBool() const { return as.boolean; }
int Value::asInt() const
{
    return as.integer;
}

uint8 Value::asByte() const
{
    return as.byte;
}

uint32 Value::asUInt() const
{
    return as.unsignedInteger;
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
    else if (type == ValueType::BYTE)
    {
        return static_cast<float>(as.byte);
    }
    else if (type == ValueType::FLOAT)
    {
        return as.n_float;
    }
    Warning("Wrong type conversion to float");
    printValueNl(*this);
    return 0;
}
const char *Value::asStringChars() const 
{
    VALUE_TYPE_CHECK(type == ValueType::STRING, "Try to get string but is %s", typeToString(type));
    String* s = StringPool::instance().getString(as.id);
    return s->chars();
}
String *Value::asString() const 
{
    VALUE_TYPE_CHECK(type == ValueType::STRING, "Try to get string but is %s", typeToString(type));
    return  StringPool::instance().getString(as.id);
    
}

int Value::asFunctionId() const 
{ 
    VALUE_TYPE_CHECK(type == ValueType::FUNCTION, "Try to get function but is %s", typeToString(type));
    return as.id; 
}
int Value::asNativeId() const 
{
    VALUE_TYPE_CHECK(type == ValueType::NATIVE, "Try to get native function but is %s", typeToString(type));
    return as.id; 
}
int Value::asProcessId() const 
{ 
    VALUE_TYPE_CHECK(type == ValueType::PROCESS, "Try to get process but is %s", typeToString(type));
    return as.id; 
}

int Value::asStructId() const
{
    VALUE_TYPE_CHECK(type == ValueType::STRUCT, "Try to get struct but is %s", typeToString(type));
    return as.id;
}

int Value::asClassId() const
{
    VALUE_TYPE_CHECK(type == ValueType::CLASSID, "Try to get class but is %s", typeToString(type));
    return as.id;
}

int Value::asClassNativeId() const
{
    VALUE_TYPE_CHECK(type == ValueType::NATIVECLASS, "Try to get native class but is %s", typeToString(type));
    return as.id;
}

void *Value::asPointer() const
{
    VALUE_TYPE_CHECK(type == ValueType::POINTER, "Try to get pointer but is %s", typeToString(type));
    return as.pointer;
}

int Value::asNativeStructId() const
{
    VALUE_TYPE_CHECK(type == ValueType::NATIVESTRUCT, "Try to get native struct but is %s", typeToString(type));
    return as.id;
}

StructInstance *Value::asStructInstance() const
{
    VALUE_TYPE_CHECK(type == ValueType::STRUCTINSTANCE, "Try to get struct but is %s", typeToString(type));
    return InstancePool::instance().getStruct(as.id);
}

ArrayInstance *Value::asArray() const
{
    VALUE_TYPE_CHECK(type == ValueType::ARRAY, "Try to get array but is %s", typeToString(type));
    return InstancePool::instance().getArray(as.id);
}

MapInstance *Value::asMap() const
{
    VALUE_TYPE_CHECK(type == ValueType::MAP, "Try to get map but is %s", typeToString(type));
    return  InstancePool::instance().getMap(as.id);
}

ClassInstance *Value::asClassInstance() const
{
    VALUE_TYPE_CHECK(type == ValueType::CLASSINSTANCE, "Try to get class but is %s", typeToString(type));
    return InstancePool::instance().getClass(as.id);
}

NativeInstance *Value::asNativeClassInstance() const
{
    VALUE_TYPE_CHECK(type == ValueType::NATIVECLASSINSTANCE, "Try to get native class but is %s", typeToString(type));

    return as.sClassInstance;
}

NativeStructInstance *Value::asNativeStructInstance() const
{
    VALUE_TYPE_CHECK(type == ValueType::NATIVESTRUCTINSTANCE, "Try to get native struct but is %s", typeToString(type));
    return as.sNativeStruct;
}

double Value::asNumber() const
{
    if (type == ValueType::DOUBLE)
    {
        return static_cast<double>(as.number);
    }
    else if (type == ValueType::INT)
    {
        return static_cast<double>(as.integer);
    }
    else if (type == ValueType::BYTE)
    {
        return static_cast<double>(as.byte);
    }
    Warning("Wrong type conversion to number");
    return 0;
}
static void printValueIndented(const Value &value, int depth = 0)
{
    switch (value.type)
    {
    case ValueType::NIL:
        printf("nil");
        break;
    case ValueType::BOOL:
        printf("%s", value.as.boolean ? "true" : "false");
        break;
    case ValueType::BYTE:
        printf("%u", value.as.byte);
        break;
    case ValueType::INT:
        printf("%d", value.as.integer);
        break;
    case ValueType::UINT:
        printf("%u", value.as.unsignedInteger);
        break;
    case ValueType::FLOAT:
        printf("%.2f", value.as.n_float);
        break;
    case ValueType::DOUBLE:
        printf("%.2f", value.as.number);
        break;
    case ValueType::STRING:
    {
        printf("<string:%d> %s", value.as.id, value.asStringChars());
    }
        break;
    case ValueType::FUNCTION:
        printf("<function:%d>", value.as.id);
        break;
    case ValueType::NATIVE:
        printf("<native>");
        break;
    case ValueType::PROCESS:
        printf("<process:%d>", value.as.id);
        break;
    case ValueType::ARRAY:
    {
        ArrayInstance *arr = value.asArray();
        printf("[");
        for (int i = 0; i < (int)arr->values.size(); i++)
        {
            printValueIndented(arr->values[i], depth + 1);
            if (i < (int)arr->values.size() - 1)
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
                printValueIndented(val, depth + 1);
                i++; });
        printf("}");
        break;
    }
    case ValueType::STRUCTINSTANCE:
    {
        StructInstance *inst = value.asStructInstance();
        printf("struct_instance{%s: ", inst->def->name->chars());
        bool first = true;
        inst->def->names.forEach([&](String *key, int idx)
                                 {
                if (!first) printf(", ");
                printf("%s=", key->chars());
                printValueIndented(inst->values[idx], depth + 1);
                first = false; });
        printf("}");
        break;
    }
    case ValueType::CLASSINSTANCE:
    {
        ClassInstance *inst = value.asClassInstance();
        printf("<class_instance:%s>", inst->klass->name->chars());
        break;
    }
    case ValueType::NATIVECLASSINSTANCE:
    {
        NativeInstance *inst = value.as.sClassInstance;
        printf("<native_class_instance:%s,%i>", inst->klass->name->chars(), inst->klass->id);
        break;
    }
    case ValueType::NATIVESTRUCTINSTANCE:
    {
        NativeStructInstance *inst = value.as.sNativeStruct;
        printf("<native_struct_instance:%s,%d>", inst->def->name->chars(), inst->def->id);
        break;
    }
    case ValueType::POINTER:
        printf("<ptr:%p>", value.as.pointer);
        break;
    case ValueType::STRUCT:
        printf("<struct:%d>", value.as.id);
        break;
    case ValueType::NATIVESTRUCT:
        printf("<native_struct:%d>", value.as.id);
        break;
    case ValueType::CLASSID:
        printf("<class:%d>", value.as.id);
        break;
    default:
        printf("<?unknown>");
    }
}

void printValue(const Value &value)
{
    printValueIndented(value, 0);
}

void printValueNl(const Value &value)
{
    printValue(value);
    printf("\n");
}

const char *typeToString(ValueType type)
{
    switch (type)
    {
    case ValueType::NIL:
        return "nil";
    case ValueType::BOOL:
        return "bool";
    case ValueType::BYTE:
        return "byte";
    case ValueType::INT:
        return "int";
    case ValueType::UINT:
        return "uint";
    case ValueType::FLOAT:
        return "float";
    case ValueType::DOUBLE:
        return "double";
    case ValueType::STRING:
        return "string";
    case ValueType::FUNCTION:
        return "function";
    case ValueType::NATIVE:
        return "native";
    case ValueType::PROCESS:
        return "process";
    case ValueType::ARRAY:
        return "array";
    case ValueType::MAP:
        return "map";
    case ValueType::STRUCT:
        return "struct";
    case ValueType::STRUCTINSTANCE:
        return "struct_instance";
    case ValueType::CLASSINSTANCE:
        return "class_instance";
    case ValueType::NATIVECLASSINSTANCE:
        return "native_class_instance";
    case ValueType::NATIVESTRUCTINSTANCE:
        return "native_struct_instance";
    default:
        return "unknown";
    }
}

bool valuesEqual(const Value &a, const Value &b)
{
    if (a.type != b.type)
        return false;

    switch (a.type)
    {
    case ValueType::NIL:
        return true;
    case ValueType::BOOL:
        return a.as.boolean == b.as.boolean;
    case ValueType::BYTE:
        return a.as.byte == b.as.byte;
    case ValueType::INT:
        return a.as.integer == b.as.integer;
    case ValueType::UINT:
        return a.as.unsignedInteger == b.as.unsignedInteger;
    case ValueType::FLOAT:
        return a.as.n_float == b.as.n_float;
    case ValueType::DOUBLE:
        return a.as.number == b.as.number;
    case ValueType::STRING:
        return a.as.id == b.as.id;
    case ValueType::ARRAY:
        return a.as.id == b.as.id;
    case ValueType::MAP:
        return a.as.id == b.as.id;
    case ValueType::STRUCTINSTANCE:
        return a.as.id == b.as.id;
    case ValueType::CLASSINSTANCE:
        return a.as.id == b.as.id;
    case ValueType::NATIVECLASSINSTANCE:
        return a.as.sClassInstance == b.as.sClassInstance;
    case ValueType::NATIVESTRUCTINSTANCE:
        return a.as.sNativeStruct == b.as.sNativeStruct;
    case ValueType::POINTER:
        return a.as.pointer == b.as.pointer;
    default:
        return false;
    }
}