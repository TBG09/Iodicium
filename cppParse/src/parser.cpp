#include "cppParse/parser.hpp"
#include <stdexcept>
#include <iostream>
#include <iomanip>

namespace cppParse {

// --- Parser Class Implementation ---

Parser::Parser(std::string program_name, std::string version)
    : m_program_name(std::move(program_name)), m_version(std::move(version)) {
    add_argument({"-h", "--help"})
        .help("shows help message and exits")
        .store_true();
}

void Parser::add_description(std::string description) {
    m_description = std::move(description);
}

Argument& Parser::add_argument(const std::vector<std::string>& flags) {
    Argument new_arg(flags);
    if (new_arg.m_is_positional) {
        m_positional_arguments.push_back(new_arg);
        return m_positional_arguments.back();
    } else {
        m_arguments.push_back(new_arg);
        size_t argument_index = m_arguments.size() - 1;
        for (const auto& flag : flags) {
            m_flag_map[flag] = argument_index;
        }
        return m_arguments.back();
    }
}

Parser& Parser::add_subparser(const std::string& name) {
    auto new_parser = std::make_unique<Parser>(name);
    auto& parser_ref = *new_parser;
    m_subparsers[name] = std::move(new_parser);
    return parser_ref;
}

bool Parser::is_subcommand_used(const std::string& name) const {
    return m_used_subcommand_name == name;
}

Parser& Parser::get_subparser(const std::string& name) {
    auto it = m_subparsers.find(name);
    if (it == m_subparsers.end()) {
        throw std::invalid_argument("Subparser '" + name + "' not found.");
    }
    return *it->second;
}

void Parser::apply_default_values() {
    for (const auto& arg : m_arguments) {
        if (arg.m_default_value.has_value() && m_parsed_values.find(arg.m_name) == m_parsed_values.end()) {
            m_parsed_values[arg.m_name] = *arg.m_default_value;
        }
    }
    for (const auto& arg : m_positional_arguments) {
        if (arg.m_default_value.has_value() && m_parsed_values.find(arg.m_name) == m_parsed_values.end()) {
            m_parsed_values[arg.m_name] = *arg.m_default_value;
        }
    }
}

void Parser::check_required_arguments() {
    for (const auto& arg : m_arguments) {
        if (arg.m_is_required && m_parsed_values.find(arg.m_name) == m_parsed_values.end()) {
            throw std::runtime_error("Required argument missing: " + arg.m_flags[0]);
        }
    }
    for (const auto& arg : m_positional_arguments) {
        if (arg.m_is_required && m_parsed_values.find(arg.m_name) == m_parsed_values.end()) {
            throw std::runtime_error("Required positional argument missing: " + arg.m_name);
        }
    }
}

void Parser::parse_args(int argc, char* argv[]) {
    std::vector<std::string> raw_args(argv + 1, argv + argc);

    // Handle global help flag first (before any other parsing logic)
    for (const auto& arg : raw_args) {
        if (arg == "-h" || arg == "--help") {
            HelpFormatter formatter(*this);
            std::cout << formatter.format() << std::endl;
            exit(0);
        }
    }

    // Find if there's a subcommand in the arguments
    std::string found_subcommand_name;
    size_t subcommand_index = raw_args.size(); // Position where subcommand was found

    for (size_t i = 0; i < raw_args.size(); ++i) {
        const std::string& arg = raw_args[i];

        // Check if this is a subcommand (not a flag and exists in subparsers)
        if (!arg.empty() && arg[0] != '-' && m_subparsers.find(arg) != m_subparsers.end()) {
            found_subcommand_name = arg;
            subcommand_index = i;
            break;
        }
    }

    // Separate arguments: everything before subcommand is for main parser
    std::vector<std::string> main_parser_args;
    std::vector<std::string> subparser_args;

    if (!found_subcommand_name.empty()) {
        // Main parser gets everything before the subcommand
        main_parser_args.assign(raw_args.begin(), raw_args.begin() + subcommand_index);
        // Subparser gets everything after the subcommand
        subparser_args.assign(raw_args.begin() + subcommand_index + 1, raw_args.end());
    } else {
        // No subcommand found, all args are for main parser
        main_parser_args = raw_args;
    }

    // Process main parser arguments
    std::vector<std::string> positional_candidates;

    for (size_t i = 0; i < main_parser_args.size(); ++i) {
        const std::string& arg = main_parser_args[i];

        auto flag_it = m_flag_map.find(arg);
        if (flag_it != m_flag_map.end()) {
            Argument& argument = m_arguments[flag_it->second];
            std::string arg_name_for_map = argument.m_name.empty() ? argument.m_flags[0] : argument.m_name;

            if (argument.m_is_store_true) {
                m_parsed_values[arg_name_for_map] = true;
            } else if (argument.m_nargs == '*') {
                std::vector<std::string> values;
                while (i + 1 < main_parser_args.size() &&
                       m_flag_map.find(main_parser_args[i + 1]) == m_flag_map.end() &&
                       (main_parser_args[i + 1].empty() || main_parser_args[i + 1][0] != '-')) {
                    values.push_back(main_parser_args[++i]);
                }
                m_parsed_values[arg_name_for_map] = values;
            } else if (argument.m_takes_value || argument.m_nargs == 1) {
                if (i + 1 < main_parser_args.size()) {
                    m_parsed_values[arg_name_for_map] = main_parser_args[++i];
                } else {
                    throw std::runtime_error("Argument " + arg + " requires a value.");
                }
            }
        } else if (!arg.empty() && arg[0] == '-') {
            // Unknown flag
            throw std::runtime_error("Unknown argument: " + arg);
        } else {
            // It's a positional argument
            positional_candidates.push_back(arg);
        }
    }

    // Process positional arguments for the main parser
    if (positional_candidates.size() > m_positional_arguments.size()) {
        throw std::runtime_error("Too many positional arguments. Expected at most " +
                                 std::to_string(m_positional_arguments.size()) +
                                 " but got " + std::to_string(positional_candidates.size()));
    }

    for (size_t i = 0; i < positional_candidates.size(); ++i) {
        Argument& pos_arg = m_positional_arguments[i];
        m_parsed_values[pos_arg.m_name] = positional_candidates[i];
    }

    // Apply defaults and check required args for the main parser
    apply_default_values();
    check_required_arguments();

    // If a subcommand was found, delegate to it
    if (!found_subcommand_name.empty()) {
        m_used_subcommand_name = found_subcommand_name;
        Parser& subparser = *m_subparsers[found_subcommand_name];

        // Prepare argv for the subparser
        std::vector<char*> sub_argv_c_str;
        sub_argv_c_str.push_back(const_cast<char*>(found_subcommand_name.c_str()));
        for (const auto& s : subparser_args) {
            sub_argv_c_str.push_back(const_cast<char*>(s.c_str()));
        }
        subparser.parse_args(static_cast<int>(sub_argv_c_str.size()), sub_argv_c_str.data());
    }
}

} // namespace cppParse