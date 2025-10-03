#include "cppParse/argument.hpp"
#include <stdexcept>
#include <algorithm>

namespace cppParse {

Argument::Argument(std::vector<std::string> flags)
    : m_flags(std::move(flags)) {
    if (m_flags.empty()) {
        throw std::invalid_argument("Argument must have at least one name.");
    }

    // Determine the canonical name for the argument
    if (!m_flags[0].empty() && m_flags[0][0] != '-') {
        // It's a positional argument, its name is its first flag
        m_is_positional = true;
        m_name = m_flags[0];
    } else {
        // It's an optional argument (flagged)
        // Find the longest flag (without leading dashes) as the canonical name
        std::string longest_stripped_flag_name;
        for (const auto& flag : m_flags) {
            if (flag.empty()) {
                continue;
            }

            std::string stripped_flag = flag;
            size_t first_char = stripped_flag.find_first_not_of('-');
            if (first_char != std::string::npos) {
                stripped_flag = stripped_flag.substr(first_char);
            } else {
                stripped_flag = "";
            }

            if (stripped_flag.length() > longest_stripped_flag_name.length()) {
                longest_stripped_flag_name = stripped_flag;
            }
        }

        if (longest_stripped_flag_name.empty()) {
            throw std::invalid_argument("Invalid flag format. Flags must have characters after dashes.");
        }

        m_name = longest_stripped_flag_name;
    }
}

Argument& Argument::help(std::string help_text) {
    m_help = std::move(help_text);
    return *this;
}

Argument& Argument::store_true() {
    if (m_is_positional) {
        throw std::invalid_argument("Positional arguments cannot use store_true().");
    }
    m_is_store_true = true;
    m_takes_value = false;
    return *this;
}

Argument& Argument::takes_value() {
    if (m_is_store_true) {
        throw std::invalid_argument("Cannot call takes_value() on a store_true argument.");
    }
    m_takes_value = true;
    m_nargs = 1;
    return *this;
}

Argument& Argument::required(bool is_required) {
    m_is_required = is_required;
    return *this;
}

Argument& Argument::default_value(ArgValue value) {
    m_default_value = std::move(value);
    return *this;
}

Argument& Argument::nargs(char num_args) {
    if (num_args != '*' && num_args != '+' && num_args != '?' && (num_args < '0' || num_args > '9')) {
        throw std::invalid_argument("nargs must be '*', '+', '?', or a digit 0-9.");
    }
    m_nargs = num_args;
    if (num_args != 0) {
        m_takes_value = true;
    }
    return *this;
}

} // namespace cppParse