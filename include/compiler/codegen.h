#ifndef IODICIUM_COMPILER_BYTECODE_COMPILER_H
#define IODICIUM_COMPILER_BYTECODE_COMPILER_H

#include <vector>
#include <string>
#include <map>
#include "codeparser/ast.h"
#include "executable/ioe_reader.h"
#include "common/opcode.h"
#include "common/logger.h"
#include "common/error.h"
#include "compiler/semantics.h"

namespace Iodicium {
    namespace Compiler {

        class BytecodeCompilerError : public Common::IodiciumError {
        public:
            BytecodeCompilerError(const std::string& message, int line = -1, int column = -1)
                : Common::IodiciumError(message, line, column) {}
        };

        // Represents a local variable in the compiler.
        struct Local {
            Codeparser::Token name;
            int depth;
        };

        class BytecodeCompiler : public Codeparser::StmtVisitor, public Codeparser::ExprVisitor {
        public:
            explicit BytecodeCompiler(Common::Logger& logger, SemanticAnalyzer& analyzer, bool obfuscate_enabled = false);
            Executable::Chunk compile(const std::vector<std::unique_ptr<Codeparser::Stmt>>& statements);

            const std::map<std::string, size_t>& getFunctionIPs() const { return m_function_ips; }

        private:
            Common::Logger& m_logger;
            SemanticAnalyzer& m_analyzer;
            Executable::Chunk m_chunk;
            bool m_obfuscate_enabled;
            std::map<std::string, std::string> m_obfuscation_map;
            int m_obfuscation_counter = 0;
            std::map<std::string, size_t> m_function_ips;
            std::map<std::string, std::vector<size_t>> m_call_fixups;
            
            // Scope management
            std::vector<Local> m_locals;
            int m_scope_depth = 0;

            void beginScope();
            void endScope();
            int resolveLocal(const Codeparser::Token& name);

            // Visitor methods
            void visit(const Codeparser::FunctionStmt& stmt) override;
            void visit(const Codeparser::FunctionDeclStmt& stmt) override;
            void visit(const Codeparser::ReturnStmt& stmt) override;
            void visit(const Codeparser::ExprStmt& stmt) override;
            void visit(const Codeparser::VarStmt& stmt) override;
            void visit(const Codeparser::ImportStmt& stmt) override;
            void visit(const Codeparser::BinaryExpr& expr) override;
            void visit(const Codeparser::GroupingExpr& expr) override;
            void visit(const Codeparser::LiteralExpr& expr) override;
            void visit(const Codeparser::VariableExpr& expr) override;
            void visit(const Codeparser::CallExpr& expr) override;
            void visit(const Codeparser::AssignExpr& expr) override;

            // Bytecode emission helpers
            void emitByte(uint8_t byte);
            void emitBytes(uint8_t byte1, uint8_t byte2);
            void emitShort(uint16_t value);
            void patchShort(size_t offset, uint16_t value);
            uint8_t makeConstant(const std::string& value);

            std::string getObfuscatedName(const std::string& original_name);
        };

    }
}

#endif //IODICIUM_COMPILER_BYTECODE_COMPILER_H
