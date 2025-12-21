#pragma once
#include "config.hpp"
#include "string.hpp"

enum class ValueType : uint8
{
  NIL,
  BOOL,
  INT,
  DOUBLE,
  STRING,
  ARRAY,
  MAP,
  FUNCTION,
  NATIVE,
  PROCESS
};

struct Value
{
  ValueType type;
  union
  {
    bool boolean;
    long integer;
    double number;
    String *string;
    int functionId;
    int nativeId;
    int processId;
  } as;

  Value();
  Value(const Value &other) = default;
  Value(Value &&other) noexcept = default;
  Value &operator=(const Value &other) = default;
  Value &operator=(Value &&other) noexcept = default;
  ~Value() = default;

  static Value makeNil();
  static Value makeBool(bool b);
  static Value makeTrue() { return makeBool(true); }
  static Value makeFalse() { return makeBool(false); }
  static Value makeInt(long i);
  static Value makeDouble(double d);
  static Value makeFloat(float f);
  static Value makeString(const char *str);
  static Value makeString(String *str);
  static Value makeFunction(int idx);
  static Value makeNative(int idx);
  static Value makeProcess(int idx);

  // Type checks
  bool isNil() const { return type == ValueType::NIL; }
  bool isBool() const { return type == ValueType::BOOL; }
  bool isInt() const { return type == ValueType::INT; }
  bool isDouble() const { return type == ValueType::DOUBLE; }
  bool isString() const { return type == ValueType::STRING; }
  bool isFunction() const { return type == ValueType::FUNCTION; }
  bool isNative() const { return type == ValueType::NATIVE; }
  bool isProcess() const { return type == ValueType::PROCESS; }

  // Conversions
  bool asBool() const;
  long asInt() const;
  double asDouble() const;
  float asFloat() const;
  const char *asStringChars() const;
  String *asString() const;
  int asFunctionId() const;
  int asNativeId() const;
  int asProcessId() const;




};

void printValue(const Value &value);
bool valuesEqual(const Value& a, const Value& b);