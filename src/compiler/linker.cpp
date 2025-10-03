#include "compiler/linker.h"
#include "codeparser/lexer.h"
#include "codeparser/parser.h"
#include "compiler/semantics.h"
#include "compiler/codegen.h"
#include <fstream>
#include <sstream>

namespace Iodicium {
    namespace Compiler {

        Linker::Linker(Common::Logger& logger) : m_logger(logger) {}

        Executable::Chunk Linker::link(const std::vector<std::string>& source_paths) {
            m_logger.info("Linker: Starting static link process for " + std::to_string(source_paths.size()) + " source files.");

            std::vector<std::unique_ptr<Codeparser::Stmt>> combined_ast;

            // --- Step 1: Parse all source files into a single AST --- 
            for (const auto& path : source_paths) {
                m_logger.debug("Linker: Parsing file: " + path);
                std::ifstream file(path);
                if (!file.is_open()) throw std::runtime_error("Could not open source file: " + path);
                std::stringstream buffer; buffer << file.rdbuf();
                std::string source = buffer.str();

                Codeparser::Lexer lexer(source, m_logger);
                auto tokens = lexer.tokenize();

                Codeparser::Parser parser(tokens, m_logger);
                auto ast = parser.parse();

                for (auto& stmt : ast) {
                    combined_ast.push_back(std::move(stmt));
                }
            }

            // --- Step 2: Perform a single semantic analysis pass on the combined AST ---
            m_logger.info("Linker: Performing global semantic analysis...");
            // We need a base path for resolving imports inside the files.
            // For now, we'll use the path of the first source file.
            std::string base_path = ".";
            if (!source_paths.empty()) {
                base_path = source_paths[0].substr(0, source_paths[0].find_last_of("/"));
            }
            SemanticAnalyzer analyzer(m_logger, base_path);
            analyzer.analyze(combined_ast);

            // --- Step 3: Perform a single code generation pass ---
            m_logger.info("Linker: Generating bytecode...");
            BytecodeCompiler compiler(m_logger, analyzer, false);
            Executable::Chunk final_chunk = compiler.compile(combined_ast);

            m_logger.info("Linker: Static linking complete.");
            return final_chunk;
        }

    }
}
