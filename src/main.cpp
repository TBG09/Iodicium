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

void compileFiles(const std::vector<std::string>& paths, const std::string& output_arg, Iodicium::Common::Logger& logger, bool obfuscate_enabled, bool is_library);
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
        case ' ': return value;
        default: throw std::runtime_error("Unknown memory unit: " + std::string(1, unit_char));
    }
}

int main(int argc, char** argv) {
#if defined(_WIN32)
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);
#endif

    cppParse::Parser parser("Iodicium", "The Iodicium Language Suite");
    parser.add_argument({"-d", "--debug"}).help("Enable debug output for the compilation and runtime process.").store_true();
    parser.add_argument({"-v", "--version"}).help("Show version information and exit.").store_true();

    auto& compile_cmd = parser.add_subparser("compile");
    compile_cmd.add_description("Compile Iodicium source files.");
    compile_cmd.add_argument({"files"}).help("One or more source files to compile.").required(true).nargs('+');
    compile_cmd.add_argument({"-o", "--output"}).help("Specify the output executable file name.").required(true);
    compile_cmd.add_argument({"-ob", "--obfuscate"}).help("Obfuscate variable names in the compiled output.").store_true();
    compile_cmd.add_argument({"--library"}).help("Compile as a library (.iodl).").store_true();

    auto& run_cmd = parser.add_subparser("run");
    run_cmd.add_description("Run an Iodicium executable file.");
    run_cmd.add_argument({"file"}).help("The .iode file to execute.").required(true);
    run_cmd.add_argument({"--memory"}).help("Set the VM memory limit (e.g., 256M).");

    Iodicium::Common::Logger main_logger;

    try {
        parser.parse_args(argc, argv);

        if (parser.get<bool>("-v")) {
            std::cout << "Iodicium v1.1.4\n";
            std::cout << "Build 0105\n";
            return 0;
        }

        if (parser.get<bool>("-d")) {
            main_logger.setLevel(Iodicium::Common::LogLevel::Debug);
        } else {
            main_logger.setLevel(Iodicium::Common::LogLevel::Info);
        }

        if (parser.is_subcommand_used("compile")) {
            auto& sub_parser = parser.get_subparser("compile");
            bool obfuscate_enabled = sub_parser.get<bool>("-ob");
            bool is_library = sub_parser.get<bool>("--library");
            compileFiles(sub_parser.get<std::vector<std::string>>("files"), sub_parser.get<std::string>("-o"), main_logger, obfuscate_enabled, is_library);
        } else if (parser.is_subcommand_used("run")) {
            auto& sub_parser = parser.get_subparser("run");
            runFile(sub_parser.get<std::string>("file"), sub_parser.get<std::string>("--memory"), main_logger);
        } else if (argc == 1) {
            std::cout << "No arguments provided. Try --help for help." << std::endl;
        }

    } catch (const Iodicium::Codeparser::LexerError& e) {
        std::cerr << "Lexer Error: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
        return 1;
    } catch (const Iodicium::Codeparser::ParserError& e) {
        std::cerr << "Parsing Error: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
        return 1;
    } catch (const Iodicium::Compiler::SemanticError& e) {
        std::cerr << "Semantic Error: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
        return 1;
    } catch (const Iodicium::Compiler::BytecodeCompilerError& e) {
        std::cerr << "Bytecode Compiler Error: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
        return 1;
    } catch (const Iodicium::Executable::IoeWriterError& e) {
        std::cerr << "IoeWriter Error: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
        return 1;
    } catch (const Iodicium::Executable::IoeReaderError& e) {
        std::cerr << "IoeReader Error: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::string error_msg_narrow = e.what();
#if defined(_WIN32)
        if (error_msg_narrow.find("Could not open file") != std::string::npos) {
            Iodicium::Common::DialogOptions dialog_options;
            dialog_options.title = L"File Error";
            dialog_options.main_instruction = L"Failed to open file.";
            dialog_options.message = std::wstring(error_msg_narrow.begin(), error_msg_narrow.end());
            dialog_options.style = Iodicium::Common::DialogStyle::Error;
            Iodicium::Common::ShowDialog(dialog_options);
        } else {
            std::cerr << "Error: " << error_msg_narrow << std::endl;
        }
#else
        std::cerr << "Error: " << error_msg_narrow << std::endl;
#endif
        return 1;
    }

    return 0;
}

void compileFiles(const std::vector<std::string>& paths, const std::string& output_arg, Iodicium::Common::Logger& logger, bool obfuscate_enabled, bool is_library) {
    Iodicium::Compiler::Linker linker(logger);
    Iodicium::Executable::Chunk chunk = linker.link(paths);

    logger.info("Writing final output to: " + output_arg);
    if (is_library) {
        Iodicium::Executable::IodlWriter writer(logger);
        // writer.setExports(...);
        writer.setCode(chunk.code);
        for(const auto& constant : chunk.constants) writer.addConstant(constant);
        writer.writeToFile(output_arg);
    } else {
        Iodicium::Executable::IoeWriter writer(logger);
        writer.setImports({});
        writer.setCode(chunk.code);
        for(const auto& constant : chunk.constants) writer.addConstant(constant);
        writer.writeToFile(output_arg);
    }

    logger.info("Compilation successful. Output written to " + output_arg);
}

void runFile(const std::string& path, const std::string& memory, Iodicium::Common::Logger& logger) {
    logger.info("Initializing Iodicium VM...");

    size_t memoryLimitBytes = 0;
    if (!memory.empty()) {
        try {
            memoryLimitBytes = parseMemoryString(memory);
            logger.debug("Parsed memory limit: " + std::to_string(memoryLimitBytes) + " bytes");
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
