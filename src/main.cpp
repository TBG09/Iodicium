// On Windows, include the main header first, then specific ones, before any other headers.
#if defined(_WIN32)
#include <windows.h>
#include <commctrl.h>
#endif

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cctype> // For std::isdigit, std::toupper

// External and shared library headers
#include "cppParse/parser.hpp"
#include "cppParse/help_formatter.hpp" // Include for manual help printing
#include "common/dialog.h"
#include "common/logger.h"
#include "executable/ioe_writer.h"
#include "executable/ioe_reader.h"
#include "executable/iodl_writer.h"

// Internal compiler headers
#include "codeparser/lexer.h"
#include "codeparser/parser.h"
#include "compiler/semantics.h" // New: Semantic Analyzer
#include "compiler/codegen.h"
#include "vm/vm.h"

// Function declarations
void compileFile(const std::string& path, const std::string& output_arg, Iodicium::Common::Logger& logger, bool obfuscate_enabled, bool is_library);
void runFile(const std::string& path, const std::string& memory, Iodicium::Common::Logger& logger);

// Helper function to parse memory string (e.g., "256M", "1G") into bytes
size_t parseMemoryString(const std::string& memory_str) {
    if (memory_str.empty()) {
        return 0; // No limit
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
    // On Windows, initialize the common controls library to enable modern dialogs.
#if defined(_WIN32)
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);
#endif

    cppParse::Parser parser("Iodicium", "The Iodicium Language Suite");
    parser.add_argument({"-d", "--debug"}).help("Enable debug output for the compilation and runtime process.").store_true();
    parser.add_argument({"-v", "--version"}).help("Show version information and exit.").store_true();

    // --- Compile Command ---
    auto& compile_cmd = parser.add_subparser("compile");
    compile_cmd.add_description("Compile an Iodicium source file.");
    compile_cmd.add_argument({"file"}).help("The Iodicium source file to compile.").required(true);
    compile_cmd.add_argument({"-o", "--output"}).help("Specify the output executable file name.");
    compile_cmd.add_argument({"-ob", "--obfuscate"}).help("Obfuscate variable names in the compiled output.").store_true();
    compile_cmd.add_argument({"--library"}).help("Compile as a library (.iodl).").store_true();

    // --- Run Command ---
    auto& run_cmd = parser.add_subparser("run");
    run_cmd.add_description("Run an Iodicium executable file.");
    run_cmd.add_argument({"file"}).help("The .iode file to execute.").required(true);
    run_cmd.add_argument({"--memory"}).help("Set the VM memory limit (e.g., 256M).");

    // Create and configure the global logger
    Iodicium::Common::Logger main_logger;

    try {
        parser.parse_args(argc, argv);

        // Handle version flag first
        if (parser.get<bool>("-v")) {
            std::cout << "Iodicium v1.1.4\n";
            std::cout << "Build 0105\n";
            return 0; // Exit after printing version
        }

        bool debug_enabled = parser.get<bool>("-d");

        if (debug_enabled) {
            main_logger.setLevel(Iodicium::Common::LogLevel::Debug);
        } else {
            main_logger.setLevel(Iodicium::Common::LogLevel::Info); // Changed to Info to show progress
        }

        cppParse::Parser* sub_parser_ptr = nullptr;

        if (parser.is_subcommand_used("compile")) {
            sub_parser_ptr = &parser.get_subparser("compile");
            bool obfuscate_enabled = sub_parser_ptr->get<bool>("-ob");
            bool is_library = sub_parser_ptr->get<bool>("--library");
            compileFile(sub_parser_ptr->get<std::string>("file"), sub_parser_ptr->get<std::string>("-o"), main_logger, obfuscate_enabled, is_library);
        } else if (parser.is_subcommand_used("run")) {
            sub_parser_ptr = &parser.get_subparser("run");
            runFile(sub_parser_ptr->get<std::string>("file"), sub_parser_ptr->get<std::string>("--memory"), main_logger);
        } else { // No subcommand used
            if (argc == 1) { // Only program name, no arguments
                std::cout << "No arguments provided. Try --help for help." << std::endl;
            }
        }

    } catch (const Iodicium::Codeparser::LexerError& e) {
        std::cerr << "Lexer Error: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
        return 1;
    } catch (const Iodicium::Codeparser::ParserError& e) {
        std::cerr << "Parsing Error: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
        return 1;
    } catch (const Iodicium::Compiler::SemanticError& e) { // Catch SemanticError
        std::cerr << "Semantic Error: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
        return 1;
    } catch (const Iodicium::Compiler::BytecodeCompilerError& e) { // Catch BytecodeCompilerError
        std::cerr << "Bytecode Compiler Error: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
        return 1;
    } catch (const Iodicium::Executable::IoeWriterError& e) { // Catch IoeWriterError
        std::cerr << "IoeWriter Error: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
        return 1;
    } catch (const Iodicium::Executable::IoeReaderError& e) { // Catch IoeReaderError
        std::cerr << "IoeReader Error: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::string error_msg_narrow = e.what();
#if defined(_WIN32)
        // Only show GUI dialog for specific file-related errors on Windows
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
        // On non-Windows, always print to stderr
        std::cerr << "Error: " << error_msg_narrow << std::endl;
#endif
        return 1;
    }

    return 0;
}

void compileFile(const std::string& path, const std::string& output_arg, Iodicium::Common::Logger& logger, bool obfuscate_enabled, bool is_library) {
    logger.info("[1/6] Reading source file: " + path);
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("Could not open file: " + path);
    std::stringstream buffer; buffer << file.rdbuf();
    std::string source = buffer.str();

    logger.info("[2/6] Tokenizing source code...");
    Iodicium::Codeparser::Lexer lexer(source, logger);
    std::vector<Iodicium::Codeparser::Token> tokens = lexer.tokenize();

    logger.info("[3/6] Building Abstract Syntax Tree (AST)...");
    Iodicium::Codeparser::Parser parser(tokens, logger);
    std::vector<std::unique_ptr<Iodicium::Codeparser::Stmt>> ast = parser.parse();

    logger.info("[4/6] Performing semantic analysis...");
    std::string base_path = path.substr(0, path.find_last_of("/"));
    Iodicium::Compiler::SemanticAnalyzer semantic_analyzer(logger, base_path);
    semantic_analyzer.analyze(ast);

    logger.info("[5/6] Compiling AST to bytecode...");
    Iodicium::Compiler::BytecodeCompiler bytecodeCompiler(logger, semantic_analyzer, obfuscate_enabled);
    Iodicium::Executable::Chunk chunk = bytecodeCompiler.compile(ast);

    std::string out_path = output_arg;
    if (out_path.empty()) {
        out_path = path.substr(0, path.rfind('.')) + (is_library ? ".iodl" : ".iode");
    }

    logger.info("[6/6] Writing output to: " + out_path);
    if (is_library) {
        Iodicium::Executable::IodlWriter writer(logger);
        writer.setExports(bytecodeCompiler.getFunctionIPs());
        writer.setCode(chunk.code);
        for(const auto& constant : chunk.constants) writer.addConstant(constant);
        writer.writeToFile(out_path);
    } else {
        Iodicium::Executable::IoeWriter writer(logger);
        writer.setImports(chunk.external_references);
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
            logger.debug("Parsed memory limit: " + std::to_string(memoryLimitBytes) + " bytes");
        } catch (const std::runtime_error& e) {
            logger.error(e.what());
            throw; // Re-throw to be caught by main's try-catch
        }
    }

    Iodicium::Executable::IoeReader reader(logger);
    Iodicium::Executable::Chunk chunk = reader.readFromFile(path);

    Iodicium::VM::VirtualMachine vm(logger, memoryLimitBytes);
    std::string base_path = path.substr(0, path.find_last_of("/"));
    vm.run(chunk, base_path);

    logger.info("Execution finished.");
}
