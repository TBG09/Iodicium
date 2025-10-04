#if defined(_WIN32)
#include <windows.h>
#include <commctrl.h>
#endif

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cctype>

#include "cppParse/parser.hpp"
#include "cppParse/help_formatter.hpp"
#include "cppToml/parser.hpp"
#include "cppToml/lexer.hpp"

#include "common/dialog.h"
#include "common/logger.h"
#include "executable/ioe_writer.h"
#include "executable/ioe_reader.h"
#include "executable/iodl_writer.h"


#include "compiler/linker.h"
#include "vm/vm.h"


#include "codeparser/lexer.h"
#include "codeparser/parser.h"
#include "compiler/semantics.h"
#include "compiler/codegen.h"

void compileProject(const std::string& project_path, Iodicium::Common::Logger& logger, bool obfuscate_enabled);
void runFile(const std::string& path, const std::string& memory, Iodicium::Common::Logger& logger);

size_t parseMemoryString(const std::string& memory_str) {
    if (memory_str.empty()) {
        return 0;
    }

    std::string num_part;
    char unit_char = ' ';
    size_t i = 0;
    while (i < memory_str.length() && std::isdigit(memory_str[i])) {
        num_part += memory_str[i];
        i++;
    }

    if (i < memory_str.length()) {
        unit_char = std::toupper(memory_str[i]);
    }

    size_t value = 0;
    try {
        value = std::stoull(num_part);
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid memory value: " + memory_str);
    }

    switch (unit_char) {
        case 'K': return value * 1024;
        case 'M': return value * 1024 * 1024;
        case 'G': return value * 1024 * 1024 * 1024;
        case ' ': return value; // Assume bytes if no unit
        default: throw std::runtime_error("Unknown memory unit: " + std::string(1, unit_char));
    }
}

int main(int argc, char** argv) {
#if defined(_WIN32)
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);
#endif

    cppParse::Parser parser("Iodicium", "1.5.6, build 208");
    parser.add_argument({"-d", "--debug"}).help("Enable debug output for the compilation and runtime process.").store_true();
    parser.add_argument({"-v", "--version"}).help("Show version information and exit.").store_true();

    // --- Compile Command ---
    auto& compile_cmd = parser.add_subparser("compile");
    compile_cmd.add_description("Compile an Iodicium project.");
    compile_cmd.add_argument({"project"}).help("Path to the Iodicium.toml project file.").required(true);
    compile_cmd.add_argument({"-ob", "--obfuscate"}).help("Obfuscate variable names in the compiled output.").store_true();
    compile_cmd.add_argument({"-h", "--help"}).help("Show this help message and exit.").store_true();

    // --- Run Command ---
    auto& run_cmd = parser.add_subparser("run");
    run_cmd.add_description("Run an Iodicium executable file.");
    run_cmd.add_argument({"file"}).help("The .iode file to execute.").required(true);
    run_cmd.add_argument({"--memory"}).help("Set the VM memory limit (e.g., 256M).");
    run_cmd.add_argument({"-h", "--help"}).help("Show this help message and exit.").store_true();

    Iodicium::Common::Logger main_logger;

    try {
        parser.parse_args(argc, argv);

        if (parser.get<bool>("-v")) {
            std::cout << "Iodicium v1.5.6\n";
            std::cout << "Build 0208\n";
            return 0;
        }

        bool debug_enabled = parser.get<bool>("-d");
        main_logger.setLevel(debug_enabled ? Iodicium::Common::LogLevel::Debug : Iodicium::Common::LogLevel::Info);

        if (parser.is_subcommand_used("compile")) {
            auto& sub_parser = parser.get_subparser("compile");
            if (sub_parser.get<bool>("--help")) {
                cppParse::HelpFormatter formatter(sub_parser);
                std::cout << formatter.format();
                return 0;
            }
            bool obfuscate_enabled = sub_parser.get<bool>("-ob");
            compileProject(sub_parser.get<std::string>("project"), main_logger, obfuscate_enabled);
        } else if (parser.is_subcommand_used("run")) {
            auto& sub_parser = parser.get_subparser("run");
            if (sub_parser.get<bool>("--help")) {
                cppParse::HelpFormatter formatter(sub_parser);
                std::cout << formatter.format();
                return 0;
            }
            runFile(sub_parser.get<std::string>("file"), sub_parser.get<std::string>("--memory"), main_logger);
        } else if (argc == 1) {
            std::cout << "No arguments provided. Try --help for help." << std::endl;
        }

    } catch (const Iodicium::Common::IodiciumError& e) {
        std::cerr << "Error";
        if (e.getLine() != -1) {
            std::cerr << " (line " << e.getLine() << ", col " << e.getColumn() << ")";
        }
        std::cerr << ": " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {

        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

void compileProject(const std::string& project_path, Iodicium::Common::Logger& logger, bool obfuscate_enabled) {
    logger.info("Compiling project: " + project_path);

    std::ifstream file(project_path);
    if (!file.is_open()) throw std::runtime_error("Could not open project file: " + project_path);
    std::stringstream buffer; buffer << file.rdbuf();
    std::string toml_source = buffer.str();

    cppToml::Lexer toml_lexer(toml_source);
    auto toml_tokens = toml_lexer.tokenize();
    cppToml::Parser toml_parser(toml_tokens);
    auto toml_ast = toml_parser.parse();

    std::string project_name = dynamic_cast<cppToml::StringValue&>(*toml_ast->top_level_values["name"]->value).value;
    std::string project_type = dynamic_cast<cppToml::StringValue&>(*toml_ast->top_level_values["type"]->value).value;
    auto& sources_node = dynamic_cast<cppToml::ArrayValue&>(*toml_ast->top_level_values["sources"]->value);
    
    std::vector<std::string> source_files;
    std::string project_base_path = project_path.substr(0, project_path.find_last_of("/"));
    for (const auto& val : sources_node.values) {
        source_files.push_back(project_base_path + "/" + dynamic_cast<cppToml::StringValue&>(*val).value);
    }

    bool is_library = (project_type == "library");

    Iodicium::Compiler::Linker linker(logger);
    Iodicium::Executable::Chunk chunk = linker.link(source_files);

    std::string out_path = project_name + (is_library ? ".iodl" : ".iode");
    logger.info("Writing final output to: " + out_path);

    if (is_library) {
        Iodicium::Executable::IodlWriter writer(logger);
        writer.setExports(linker.getFunctionIPs());
        writer.setCode(chunk.code);
        for(const auto& constant : chunk.constants) writer.addConstant(constant);
        writer.writeToFile(out_path);
    } else {
        Iodicium::Executable::IoeWriter writer(logger);
        writer.setImports({});
        writer.setCode(chunk.code);
        for(const auto& constant : chunk.constants) writer.addConstant(constant);
        writer.writeToFile(out_path);
    }

    logger.info("Compilation successful. Output written to " + out_path);
}

void runFile(const std::string& path, const std::string& memory, Iodicium::Common::Logger& logger) {
    logger.info("Initializing Iodicium VM...");

    size_t memoryLimitBytes = 0;
    if (!memory.empty()) {
        try {
            memoryLimitBytes = parseMemoryString(memory);
        } catch (const std::runtime_error& e) {
            logger.error(e.what());
            throw;
        }
    }

    Iodicium::Executable::IoeReader reader(logger);
    Iodicium::Executable::Chunk chunk = reader.readFromFile(path);

    Iodicium::VM::VirtualMachine vm(logger, memoryLimitBytes);
    vm.run(chunk);

    logger.info("Execution finished.");
}
