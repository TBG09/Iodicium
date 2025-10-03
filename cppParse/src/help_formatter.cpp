#include "cppParse/help_formatter.hpp"
#include "cppParse/parser.hpp"
#include "cppParse/argument.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace cppParse {

HelpFormatter::HelpFormatter(const Parser& parser)
    : m_parser(parser) {}

std::string HelpFormatter::format() const {
    std::stringstream ss;

    // Usage line
    ss << "Usage: " << m_parser.m_program_name;

    // Add optional arguments indicator if any exist
    if (!m_parser.m_arguments.empty()) {
        ss << " [options]";
    }

    // Add positional arguments
    if (!m_parser.m_positional_arguments.empty()) {
        for (const auto& arg : m_parser.m_positional_arguments) {
            ss << " ";
            if (!arg.m_is_required) {
                ss << "[";
            }
            ss << arg.m_name;
            if (!arg.m_is_required) {
                ss << "]";
            }
        }
    }

    // Add subcommands indicator
    if (!m_parser.m_subparsers.empty()) {
        ss << " <subcommand> [subcommand-options]";
    }

    ss << std::endl;

    // Description
    if (!m_parser.m_description.empty()) {
        ss << std::endl << m_parser.m_description << std::endl;
    }

    // Lambda to format argument lists
    auto format_args = [&](const std::vector<Argument>& args, bool show_required = true) {
        // Find the longest flag string for alignment
        size_t max_flag_width = 0;
        for (const auto& arg : args) {
            size_t flag_width = 0;
            for (size_t i = 0; i < arg.m_flags.size(); ++i) {
                flag_width += arg.m_flags[i].length();
                if (i < arg.m_flags.size() - 1) {
                    flag_width += 2; // ", "
                }
            }
            max_flag_width = std::max(max_flag_width, flag_width);
        }

        // Ensure minimum width and add padding
        max_flag_width = std::max(max_flag_width, size_t(10)) + 4;

        for (const auto& arg : args) {
            ss << "  ";

            // Build flags string
            std::string flags_str;
            for (size_t i = 0; i < arg.m_flags.size(); ++i) {
                flags_str += arg.m_flags[i];
                if (i < arg.m_flags.size() - 1) {
                    flags_str += ", ";
                }
            }

            ss << std::left << std::setw(max_flag_width) << flags_str;

            // Help text
            ss << arg.m_help;

            // Show if required
            if (show_required && arg.m_is_required) {
                ss << " (required)";
            }

            // Show default value if present
            if (arg.m_default_value.has_value()) {
                ss << " (default: ";
                if (auto* bool_val = std::get_if<bool>(&*arg.m_default_value)) {
                    ss << (*bool_val ? "true" : "false");
                } else if (auto* str_val = std::get_if<std::string>(&*arg.m_default_value)) {
                    ss << *str_val;
                } else if (auto* vec_val = std::get_if<std::vector<std::string>>(&*arg.m_default_value)) {
                    ss << "[";
                    for (size_t i = 0; i < vec_val->size(); ++i) {
                        ss << (*vec_val)[i];
                        if (i < vec_val->size() - 1) ss << ", ";
                    }
                    ss << "]";
                }
                ss << ")";
            }

            ss << std::endl;
        }
    };

    // Positional arguments section
    if (!m_parser.m_positional_arguments.empty()) {
        ss << std::endl << "Positional Arguments:" << std::endl;
        format_args(m_parser.m_positional_arguments);
    }

    // Optional arguments section
    if (!m_parser.m_arguments.empty()) {
        ss << std::endl << "Optional Arguments:" << std::endl;
        format_args(m_parser.m_arguments, false);
    }

    // Subcommands section
    if (!m_parser.m_subparsers.empty()) {
        ss << std::endl << "Subcommands:" << std::endl;

        // Find longest subcommand name
        size_t max_name_width = 0;
        for (const auto& pair : m_parser.m_subparsers) {
            max_name_width = std::max(max_name_width, pair.first.length());
        }
        max_name_width = std::max(max_name_width, size_t(10)) + 4;

        for (const auto& pair : m_parser.m_subparsers) {
            ss << "  " << std::left << std::setw(max_name_width) << pair.first;
            if (pair.second->m_description.empty()) {
                ss << "(no description)" << std::endl;
            } else {
                ss << pair.second->m_description << std::endl;
            }
        }
    }

    // Version info
    if (!m_parser.m_version.empty()) {
        ss << std::endl << "Version: " << m_parser.m_version << std::endl;
    }

    return ss.str();
}

}