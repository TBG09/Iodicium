#pragma once

#include "cppToml/tokens.hpp"
#include "cppToml/value.hpp" // Include the new Value system
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace cppToml {

// Base class for all AST nodes
struct Node {
    virtual ~Node() = default;
};

// Represents a key = value pair
struct KeyValueNode : public Node {
    std::string key;
    std::unique_ptr<Value> value; // Changed to hold a parsed Value object

    KeyValueNode(std::string key, std::unique_ptr<Value> val)
        : key(std::move(key)), value(std::move(val)) {}
};

// Represents a table, which contains key-value pairs
struct TableNode : public Node {
    std::string name;
    std::map<std::string, std::unique_ptr<KeyValueNode>> values;
};

// Represents the root of the TOML document
struct RootNode : public Node {
    std::map<std::string, std::unique_ptr<TableNode>> tables;
    std::map<std::string, std::unique_ptr<KeyValueNode>> top_level_values;
};

}
