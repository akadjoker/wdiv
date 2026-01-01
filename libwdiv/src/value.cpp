#include "value.hpp"
#include "interpreter.hpp"
#include "pool.hpp"
#include "instances.hpp"

Value::Value() : type(ValueType::NIL)
{
    as.byte = 0;
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
    v.as.integer = idx;
    return v;
}

Value Value::makeNative(int idx)
{
    Value v;
    v.type = ValueType::NATIVE;
    v.as.integer = idx;
    return v;
}

Value Value::makeNativeClass(int idx)
{
    Value v;
    v.type = ValueType::NATIVECLASS;
    v.as.integer = idx;
    return v;
}

Value Value::makeNativeClassInstance()
{
    Value v;
    v.type = ValueType::NATIVECLASSINSTANCE;
    v.as.sClassInstance = InstancePool::instance().createNativeClass();
    return v;
}

Value Value::makeProcess(int idx)
{
    Value v;
    v.type = ValueType::PROCESS;
    v.as.integer = idx;
    return v;
}

Value Value::makeStruct(int idx)
{
    Value v;
    v.type = ValueType::STRUCT;
    v.as.integer = idx;
    return v;
}

Value Value::makeStructInstance()
{
    Value v;
    v.type = ValueType::STRUCTINSTANCE;
    v.as.sInstance = InstancePool::instance().createStruct();
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

Value Value::makeClass(int idx)
{
    Value v;
    v.type = ValueType::CLASS;
    v.as.integer = idx;
    return v;
}

Value Value::makeClassInstance()
{
    ClassInstance *c = InstancePool::instance().creatClass();
    Value v;
    v.type = ValueType::CLASSINSTANCE;
    v.as.integer = c->index;
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
    v.as.integer = idx;
    return v;
}

Value Value::makeNativeStructInstance()
{
    Value v;
    v.type = ValueType::NATIVESTRUCTINSTANCE;
    v.as.sNativeStruct = InstancePool::instance().createNativeStruct();
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
    v.as.real = f;
    return v;
}

bool Value::isNumber() const
{
    return ((type == ValueType::INT) || (type == ValueType::DOUBLE) || (type == ValueType::BYTE) || (type == ValueType::FLOAT));
}

bool Value::asBool() const 
{ 
    if (type == ValueType::NIL)
    {
        return false;
    } else if (type == ValueType::BOOL)
    {
        return as.boolean;
    } else if (type == ValueType::INT)
    {
        return as.integer != 0;
    } else if (type == ValueType::BYTE)
    {
        return as.byte != 0;
    } else if (type == ValueType::DOUBLE)
    {
        return as.number != 0.0;
    } else if (type == ValueType::FLOAT)
    {
        return as.real != 0.0f;
    }
    return as.boolean; 
}
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
    else if (type == ValueType::BYTE)
    {
        return static_cast<double>(as.byte);
    }
    else if (type == ValueType::FLOAT)
    {
        return static_cast<double>(as.real);
    } else if (type == ValueType::UINT)
    {
        return static_cast<double>(as.unsignedInteger);
    } else if (type == ValueType::BOOL)
    {
        return static_cast<double>(as.boolean);
    }
    Warning("Wrong type conversion to double");
    printValueNl(*this);
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
        return as.real;
    } else if (type == ValueType::UINT)
    {
        return static_cast<float>(as.unsignedInteger);
    } else if (type == ValueType::BOOL)
    {
        return static_cast<float>(as.boolean);
    }

    Warning("Wrong type conversion to float");
    return 0;
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
    printValueNl(*this);
    return 0;
}


bool valuesEqual(const Value &a, const Value &b)
{
     
    if ((a.isInt() || a.isDouble()) && (b.isInt() || b.isDouble())) 
    {
        double da = a.isInt() ? a.asInt() : a.asDouble();
        double db = b.isInt() ? b.asInt() : b.asDouble();
        return da == db;
    }
    
    // Resto precisa tipos iguais
    if (a.type != b.type)
        return false;
        
    switch (a.type) {
        case ValueType::BOOL:
            return a.asBool() == b.asBool();
        case ValueType::NIL:
            return true;
        case ValueType::STRING:
            return a.asString() == b.asString();
        case ValueType::STRUCT:
        case ValueType::MAP:
        case ValueType::ARRAY:
        case ValueType::FUNCTION:
        case ValueType::PROCESS:
        case ValueType::CLASS:
            return a.type == b.type;  // Já comparou antes
        default:
            return false;
    }
}
 

const char *Value::asStringChars() const { return as.string->chars(); }
String *Value::asString() const { return as.string; }

int Value::asFunctionId() const { return as.integer; }
int Value::asNativeId() const { return as.integer; }
int Value::asProcessId() const { return as.integer; }

int Value::asStructId() const
{
    return as.integer;
}

int Value::asClassId() const
{
    return as.integer;
}

int Value::asClassNativeId() const
{
    return as.integer;
}

void *Value::asPointer() const
{
    return as.pointer;
}

int Value::asNativeStructId() const
{
    return as.integer;
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

ClassInstance *Value::asClassInstance() const
{
    return InstancePool::instance().getClass(as.integer);
}

NativeInstance *Value::asNativeClassInstance() const
{
    return as.sClassInstance;
}

NativeStructInstance *Value::asNativeStructInstance() const
{
    return as.sNativeStruct;
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
    case ValueType::BYTE:
        printf("%d", value.as.byte);
        break;
    case ValueType::INT:
        printf("%d", value.as.integer);
        break;
    case ValueType::UINT:
        printf("%u", value.as.unsignedInteger);
        break;
    case ValueType::FLOAT:
        printf("%f.4f", value.as.real);
        break;
    case ValueType::DOUBLE:
        printf("%f.4f", value.as.number);
        break;
    case ValueType::STRING:
        printf("%s", value.as.string->chars());
        break;
    case ValueType::FUNCTION:
    {
        int Id = value.asFunctionId();
        printf("<function %d>", Id);
        break;
    }
    case ValueType::NATIVE:
        printf("<native>");
        break;
    case ValueType::PROCESS:
        printf("<process>");
        break;

    case ValueType::ARRAY:
    {
        ArrayInstance *arr = value.asArray();
        printf("[");
        for (int i = 0; i < (int)arr->values.size(); i++)
        {
            printValue(arr->values[i]);
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
        printValue(val);
        i++; });

        printf("}");
        break;
    }

    case ValueType::STRUCT:
    {
        int Id = value.asStructId();
        printf("<struct %d>", Id);
        break;
    }
    case ValueType::STRUCTINSTANCE:
    {
        StructInstance *instance = value.as.sInstance;
        printf("struct '%s' [", instance->def->name->chars());

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
        break;
    }
    case ValueType::CLASS:
    {
        int classId = value.asClassId();
        printf("<class %d>", classId);
        break;
    }
    case ValueType::CLASSINSTANCE:
    {
        ClassInstance *inst = value.asClassInstance();
        printf("<instance %s>", inst->klass->name->chars());
        break;
    }
    case ValueType::NATIVECLASSINSTANCE:
    {
        NativeInstance *inst = value.as.sClassInstance;
        printf("<native_instance %s>", inst->klass->name->chars());
        break;
    }
    case ValueType::NATIVESTRUCTINSTANCE:
    {
        NativeStructInstance *inst = value.as.sNativeStruct;
        printf("<native_struct_instance %s>", inst->def->name->chars());
        break;
    }
    case ValueType::POINTER:
    {

        printf("<pointer %p>", value.as.pointer);
        break;
    }

    default:
        printf("<?%ld?>",(int)(value.type));
        break;
    }
}

void printValueNl(const Value &value)
{
    printValue(value);
    printf("\n");
}
