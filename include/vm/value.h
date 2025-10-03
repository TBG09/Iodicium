#ifndef IODICIUM_VM_VALUE_H
#define IODICIUM_VM_VALUE_H

#include <string>
#include <variant>
#include <iostream>
#include <stdexcept>

namespace Iodicium {
    namespace VM {

        // Our multi-type Value struct for the VM stack and variables.
        // It can hold a boolean, a double (for all numbers), or a string.
        struct Value {
            std::variant<bool, double, std::string> data;

            // Constructors for easy initialization
            Value() : data(false) {} // Default to boolean false
            explicit Value(bool b) : data(b) {}
            explicit Value(double d) : data(d) {}
            explicit Value(std::string s) : data(std::move(s)) {}

            // Type checking helpers
            bool isBool() const { return std::holds_alternative<bool>(data); }
            bool isNumber() const { return std::holds_alternative<double>(data); }
            bool isString() const { return std::holds_alternative<std::string>(data); }

            // Value access helpers (with error checking)
            bool asBool() const {
                if (!isBool()) throw std::runtime_error("Value is not a boolean.");
                return std::get<bool>(data);
            }
            double asNumber() const {
                if (!isNumber()) throw std::runtime_error("Value is not a number.");
                return std::get<double>(data);
            }
            const std::string& asString() const {
                if (!isString()) throw std::runtime_error("Value is not a string.");
                return std::get<std::string>(data);
            }

            // Helper to get the type as a string (for debugging)
            std::string getTypeString() const {
                if (std::holds_alternative<bool>(data)) return "bool";
                if (std::holds_alternative<double>(data)) return "double";
                if (std::holds_alternative<std::string>(data)) return "string";
                return "unknown";
            }

            // Overload for easy printing/debugging
            friend std::ostream& operator<<(std::ostream& os, const Value& value);

            // toString method for logging
            std::string toString() const;
        };

    }
}

#endif //IODICIUM_VM_VALUE_H
