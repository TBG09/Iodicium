#pragma once

#include <string>
#include <vector>
#include <variant>
#include <optional>

// Cache-busting comment

namespace cppParse {

class Parser; // Forward declaration

using ArgValue = std::variant<bool, std::string, std::vector<std::string>>;

class Argument {
public:
    Argument& help(std::string help_text);
    Argument& store_true();
    Argument& takes_value();
    Argument& required(bool is_required = true);
    Argument& default_value(ArgValue value);
    Argument& nargs(char num_args);

private:
    friend class Parser;
    friend class HelpFormatter;
    Argument(std::vector<std::string> flags);

    std::vector<std::string> m_flags;
    std::string m_name;
    std::string m_help;
    bool m_is_positional = false;
    bool m_is_store_true = false;
    bool m_takes_value = false;
    bool m_is_required = false;
    char m_nargs = 0; // 0 for flags, 1 for single value, '*' for many
    std::optional<ArgValue> m_default_value;
};

}
