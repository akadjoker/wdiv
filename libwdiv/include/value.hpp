#pragma once
#include "config.hpp"
#include "string.hpp"

struct StructInstance;
struct ArrayInstance;
struct MapInstance;
struct ClassInstance;
struct NativeInstance;
struct NativeStructInstance;


enum class ValueType : uint8
{
  NIL,
  BOOL,
  CHAR,
  BYTE,
  INT,
  UINT,
  LONG,
  ULONG,
  FLOAT,
  DOUBLE,
  STRING,
  ARRAY,
  MAP,
  STRUCT,
  STRUCTINSTANCE,
  FUNCTION,
  NATIVE,
  NATIVECLASS,
  NATIVECLASSINSTANCE,
  NATIVESTRUCT,
  NATIVESTRUCTINSTANCE,
  CLASS,
  CLASSINSTANCE,
  PROCESS,
  POINTER,
};

struct Value
{
  ValueType type;
  union
  {
    bool boolean;
    uint8 byte;
    int integer;
    float real;
    double number;
    String *string;

 
    uint32 unsignedInteger;
    StructInstance *sInstance;
    ArrayInstance  *array;
    MapInstance    *map;

    NativeInstance *sClassInstance;
    NativeStructInstance   *sNativeStruct;
    void* pointer;

 
  } as;

  Value();
  Value(const Value &other) = default;
  Value(Value &&other) noexcept = default;
  Value &operator=(const Value &other) = default;
  Value &operator=(Value &&other) noexcept = default;
  // ~Value() ;

  static Value makeNil();
  static Value makeBool(bool b);
  static Value makeTrue() { return makeBool(true); }
  static Value makeFalse() { return makeBool(false); }
  static Value makeByte(uint8 b);
  static Value makeInt(int i);
  static Value makeUInt(uint32 i);
  static Value makeDouble(double d);
  static Value makeFloat(float f);
  static Value makeString(const char *str);
  static Value makeString(String *str);
  static Value makeFunction(int idx);
  static Value makeNative(int idx);
  static Value makeNativeClass(int idx);
  static Value makeNativeClassInstance();
  static Value makeProcess(int idx);
  static Value makeStruct(int idx);
  static Value makeStructInstance( );
  static Value makeMap( );
  static Value makeArray();

  static Value makeClass(int idx);
  static Value makeClassInstance();
  static Value makePointer(void* pointer);
  static Value makeNativeStruct(int idx);
  static Value makeNativeStructInstance();

  // Type checks
  bool isNumber() const;
  bool isNil() const { return type == ValueType::NIL; }
  bool isBool() const { return type == ValueType::BOOL; }
  bool isInt() const { return type == ValueType::INT; }
  bool isByte() const { return type == ValueType::BYTE; }
  bool isDouble() const { return type == ValueType::DOUBLE; }
  bool isFloat() const { return type == ValueType::FLOAT; }
  bool isUInt() const { return type == ValueType::UINT; }
  bool isString() const { return type == ValueType::STRING; }
  bool isFunction() const { return type == ValueType::FUNCTION; }
  bool isNative() const { return type == ValueType::NATIVE; }
  bool isNativeClass() const { return type == ValueType::NATIVECLASS; }
  bool isProcess() const { return type == ValueType::PROCESS; }
  bool isStruct() const { return type == ValueType::STRUCT; }
  bool isStructInstance() const { return type == ValueType::STRUCTINSTANCE; }
  bool isMap() const { return type == ValueType::MAP; }
  bool isArray() const { return type == ValueType::ARRAY; }
  bool isClass() const { return type == ValueType::CLASS; }
  bool isClassInstance(){ return type == ValueType::CLASSINSTANCE; }
  bool isNativeClassInstance(){ return type == ValueType::NATIVECLASSINSTANCE; }
  bool isPointer(){ return type == ValueType::POINTER; }
  bool isNativeStruct(){ return type == ValueType::NATIVESTRUCT; }
  bool isNativeStructInstance(){ return type == ValueType::NATIVESTRUCTINSTANCE; }

  // Conversions
  bool asBool() const;
  int asInt() const;
  uint8 asByte() const;
  uint32 asUInt() const;
  double asDouble() const;
  float asFloat() const;
  double asNumber() const;
  const char *asStringChars() const;
  int asFunctionId() const;
  int asNativeId() const;
  int asProcessId() const;
  int asStructId() const;
  int asClassId() const;
  int asClassNativeId() const;
  void* asPointer() const;
  int asNativeStructId() const;
 

  int toInt();
  float toFloat();
  double toDouble();
  
  String *asString() const;
  StructInstance* asStructInstance() const;
  ArrayInstance* asArray() const;
  MapInstance* asMap() const;
  ClassInstance *asClassInstance() const;
  NativeInstance *asNativeClassInstance() const;
  NativeStructInstance *asNativeStructInstance() const;

};

void printValue(const Value &value);
bool valuesEqual(const Value &a, const Value &b);
void printValueNl(const Value &value);