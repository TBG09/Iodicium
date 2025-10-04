#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>

namespace cppToml {

// Enum to identify the type of value held
enum class ValueType {
    STRING,
    INTEGER,
    FLOAT,
    BOOLEAN,
    DATETIME,
    ARRAY,
    TABLE
};

// Base class for all value types
struct Value {
    virtual ~Value() = default;
    virtual ValueType getType() const = 0;
};

// Derived classes for each specific TOML data type

struct StringValue : public Value {
    std::string value;
    ValueType getType() const override { return ValueType::STRING; }
    explicit StringValue(std::string val) : value(std::move(val)) {}
};

struct IntegerValue : public Value {
    int64_t value;
    ValueType getType() const override { return ValueType::INTEGER; }
    explicit IntegerValue(int64_t val) : value(val) {}
};

struct FloatValue : public Value {
    double value;
    ValueType getType() const override { return ValueType::FLOAT; }
    explicit FloatValue(double val) : value(val) {}
};

struct BooleanValue : public Value {
    bool value;
    ValueType getType() const override { return ValueType::BOOLEAN; }
    explicit BooleanValue(bool val) : value(val) {}
};

struct DateTimeValue : public Value {
    // For now, we can just store the string representation
    std::string value;
    ValueType getType() const override { return ValueType::DATETIME; }
    explicit DateTimeValue(std::string val) : value(std::move(val)) {}
};

struct ArrayValue : public Value {
    std::vector<std::unique_ptr<Value>> values;
    ValueType getType() const override { return ValueType::ARRAY; }
};

// Forward declaration for TableValue
struct KeyValueNode;

struct TableValue : public Value {
    std::map<std::string, std::unique_ptr<KeyValueNode>> values;
    ValueType getType() const override { return ValueType::TABLE; }
};

}
