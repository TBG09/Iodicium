#pragma once

#include "cppParse/argument.hpp"
#include "cppParse/help_formatter.hpp"

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>
#include <memory>
#include <algorithm>
#include <iostream>

namespace cppParse {

class Parser {
public:
    friend class HelpFormatter;

    Parser(std::string program_name, std::string version = "");

    void add_description(std::string description);
    Argument& add_argument(const std::vector<std::string>& flags);
    Parser& add_subparser(const std::string& name);

    void parse_args(int argc, char* argv[]);

    bool is_subcommand_used(const std::string& name) const;
    Parser& get_subparser(const std::string& name);

    // Check if an argument was provided (not just defaulted)
    bool has(const std::string& name) const {
        return m_parsed_values.find(name) != m_parsed_values.end();
    }

    // Get typed value with better error handling
    template<typename T>
    T get(const std::string& name) const {
        // First, try to find by the name directly
        auto val_it = m_parsed_values.find(name);

        // If not found, check if 'name' is actually a flag (starts with '-')
        // and look up the actual argument name
        if (val_it == m_parsed_values.end() && !name.empty() && name[0] == '-') {
            auto flag_it = m_flag_map.find(name);
            if (flag_it != m_flag_map.end()) {
                const Argument& arg = m_arguments[flag_it->second];
                val_it = m_parsed_values.find(arg.m_name);
            }
        }

        if (val_it == m_parsed_values.end()) {
            // Not found in parsed values - check if argument exists and return default
            bool arg_exists = false;
            for (const auto& arg : m_arguments) {
                if (arg.m_name == name || std::find(arg.m_flags.begin(), arg.m_flags.end(), name) != arg.m_flags.end()) {
                    arg_exists = true;
                    break;
                }
            }
            if (!arg_exists) {
                for (const auto& arg : m_positional_arguments) {
                    if (arg.m_name == name) {
                        arg_exists = true;
                        break;
                    }
                }
            }

            if (!arg_exists) {
                throw std::runtime_error("Unknown argument: " + name);
            }

            // Argument exists but wasn't provided - return default value
            return T{};
        }

        try {
            if (auto* value = std::get_if<T>(&val_it->second)) {
                return *value;
            }

            // Attempt type conversion for common cases
            if constexpr (std::is_same_v<T, bool>) {
                if (auto* str_val = std::get_if<std::string>(&val_it->second)) {
                    return !str_val->empty();
                }
            } else if constexpr (std::is_same_v<T, std::string>) {
                if (auto* bool_val = std::get_if<bool>(&val_it->second)) {
                    return *bool_val ? "true" : "false";
                }
            }

            throw std::runtime_error("Type mismatch for argument: " + name);
        } catch (const std::bad_variant_access&) {
            throw std::runtime_error("Invalid type requested for argument: " + name);
        }
    }

    // Get with default value if not found
    template<typename T>
    T get_or(const std::string& name, const T& default_value) const {
        auto val_it = m_parsed_values.find(name);
        if (val_it == m_parsed_values.end()) {
            return default_value;
        }

        try {
            if (auto* value = std::get_if<T>(&val_it->second)) {
                return *value;
            }
            return default_value;
        } catch (...) {
            return default_value;
        }
    }

    // Get all parsed argument names
    std::vector<std::string> get_parsed_args() const {
        std::vector<std::string> result;
        for (const auto& pair : m_parsed_values) {
            result.push_back(pair.first);
        }
        return result;
    }

    // Print help manually
    void print_help() const {
        HelpFormatter formatter(*this);
        std::cout << formatter.format() << std::endl;
    }

private:
    void check_required_arguments();
    void apply_default_values();

    std::string m_program_name;
    std::string m_version;
    std::string m_description;

    std::vector<Argument> m_arguments;
    std::map<std::string, size_t> m_flag_map;

    std::vector<Argument> m_positional_arguments;

    std::map<std::string, std::unique_ptr<Parser>> m_subparsers;
    std::string m_used_subcommand_name;

    std::map<std::string, ArgValue> m_parsed_values;
};

} // namespace cppParse