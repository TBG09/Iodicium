#include "compiler/codegen.h"
#include <stdexcept>
#include <algorithm>

namespace Iodicium {
    namespace Compiler {

        using Executable::Chunk;

        BytecodeCompiler::BytecodeCompiler(Common::Logger& logger, SemanticAnalyzer& analyzer, bool obfuscate_enabled) 
            : m_logger(logger), m_analyzer(analyzer), m_obfuscate_enabled(obfuscate_enabled), m_obfuscation_counter(0) {
            m_logger.debug("BytecodeCompiler constructor called. Obfuscation enabled: " + std::string(obfuscate_enabled ? "true" : "false") + ".");
        }

        Chunk BytecodeCompiler::compile(const std::vector<std::unique_ptr<Codeparser::Stmt>>& statements) {
            m_logger.debug("BytecodeCompiler: Starting bytecode compilation.");
            m_chunk = Executable::Chunk();
            m_obfuscation_map.clear();
            m_obfuscation_counter = 0;

            for (const auto& statement : statements) {
                statement->accept(*this);
            }
            emitByte(OP_RETURN);
            m_logger.debug("BytecodeCompiler: Finished bytecode compilation.");
            return m_chunk;
        }

        // --- Visitor Implementations ---

        void BytecodeCompiler::visit(const Codeparser::ImportStmt& stmt) {
            // The semantic analyzer handles imports. The code generator can ignore them.
            m_logger.debug("BytecodeCompiler: Skipping ImportStmt.");
        }

        void BytecodeCompiler::visit(const Codeparser::FunctionStmt& stmt) {
            m_logger.debug("BytecodeCompiler: Visiting FunctionStmt for '" + stmt.name.lexeme + "'.");
            // Record the starting IP of this function's bytecode
            size_t function_ip = m_chunk.code.size();
            m_function_ips[stmt.name.lexeme] = function_ip;

            // Compile the function body
            for (const auto& statement : stmt.body) {
                statement->accept(*this);
            }
            // Every function implicitly returns at the end
            emitByte(OP_RETURN);
        }

        void BytecodeCompiler::visit(const Codeparser::ReturnStmt& stmt) {
            m_logger.debug("BytecodeCompiler: Visiting ReturnStmt.");
            if (stmt.value) {
                stmt.value->accept(*this);
            }
            emitByte(OP_RETURN);
        }

        void BytecodeCompiler::visit(const Codeparser::ExprStmt& stmt) {
            m_logger.debug("BytecodeCompiler: Visiting ExprStmt.");
            stmt.expression->accept(*this);
        }

        void BytecodeCompiler::visit(const Codeparser::LiteralExpr& expr) {
            m_logger.debug("BytecodeCompiler: Visiting LiteralExpr (value: " + expr.value.lexeme + ").");
            uint8_t const_index = makeConstant(expr.value.lexeme);
            emitBytes(OP_CONST, const_index);
        }

        void BytecodeCompiler::visit(const Codeparser::CallExpr& expr) {
            m_logger.debug("BytecodeCompiler: Visiting CallExpr.");
            if (auto* callee = dynamic_cast<Codeparser::VariableExpr*>(expr.callee.get())) {
                // Handle built-in functions
                if (callee->name.lexeme == "writeOut") {
                    m_logger.debug("BytecodeCompiler: Compiling built-in writeOut call.");
                    expr.arguments[0]->accept(*this); // Assume one argument
                    emitByte(OP_WRITE_OUT);
                    return;
                } else if (callee->name.lexeme == "writeErr") {
                    m_logger.debug("BytecodeCompiler: Compiling built-in writeErr call.");
                    expr.arguments[0]->accept(*this); // Assume one argument
                    emitByte(OP_WRITE_ERR);
                    return;
                } else if (callee->name.lexeme == "flush") {
                    m_logger.debug("BytecodeCompiler: Compiling built-in flush call.");
                    emitByte(OP_FLUSH);
                    return;
                }

                // Check if it's an external or local function
                Symbol* symbol = m_analyzer.getSymbolTable().find(callee->name.lexeme);
                if (symbol && symbol->is_external) {
                    m_logger.debug("BytecodeCompiler: Compiling external function call to '" + callee->name.lexeme + "'.");
                    const auto& imported_modules = m_analyzer.getImportedModules();
                    std::string module_path = imported_modules[symbol->module_index];
                    std::string signature = module_path + ";" + callee->name.lexeme;
                    uint8_t ordinal = makeExternalReference(signature);
                    emitBytes(OP_EXECUTE_EXTERNAL, ordinal);
                    return;
                }

                // If it's a local function, we need to implement OP_CALL and jump to its address
                // This part is still a placeholder.
                throw BytecodeCompilerError("Bytecode compilation for local function calls not yet implemented.", expr.callee->token.line, expr.callee->token.column);

            }
            throw BytecodeCompilerError("Invalid callee expression.", expr.callee->token.line, expr.callee->token.column);
        }

        void BytecodeCompiler::visit(const Codeparser::VarStmt& stmt) {
            m_logger.debug("BytecodeCompiler: Visiting VarStmt for '" + stmt.name.lexeme + "'.");
            if (stmt.initializer) {
                stmt.initializer->accept(*this);
            } else {
                uint8_t const_index = makeConstant("");
                emitBytes(OP_CONST, const_index);
            }
            std::string var_name = getObfuscatedName(stmt.name.lexeme);
            uint8_t name_index = makeConstant(var_name);
            emitBytes(OP_DEFINE_GLOBAL, name_index);
        }

        void BytecodeCompiler::visit(const Codeparser::VariableExpr& expr) {
            m_logger.debug("BytecodeCompiler: Visiting VariableExpr for '" + expr.name.lexeme + "'.");
            std::string var_name = getObfuscatedName(expr.name.lexeme);
            uint8_t name_index = makeConstant(var_name);
            emitBytes(OP_GET_GLOBAL, name_index);
        }

        void BytecodeCompiler::visit(const Codeparser::AssignExpr& expr) {
            m_logger.debug("BytecodeCompiler: Visiting AssignExpr for '" + expr.name.lexeme + "'.");
            expr.value->accept(*this);
            std::string var_name = getObfuscatedName(expr.name.lexeme);
            uint8_t name_index = makeConstant(var_name);
            emitBytes(OP_SET_GLOBAL, name_index);
        }

        void BytecodeCompiler::visit(const Codeparser::BinaryExpr& expr) {
            m_logger.debug("BytecodeCompiler: Visiting BinaryExpr.");
            expr.left->accept(*this);
            expr.right->accept(*this);

            switch (expr.op.type) {
                case Codeparser::TokenType::PLUS:       emitByte(OP_ADD); break;
                case Codeparser::TokenType::MINUS:      emitByte(OP_SUBTRACT); break;
                case Codeparser::TokenType::STAR:       emitByte(OP_MULTIPLY); break;
                case Codeparser::TokenType::SLASH:      emitByte(OP_DIVIDE); break;
                default: throw BytecodeCompilerError("Unsupported binary operator.", expr.op.line, expr.op.column);
            }
        }

        void BytecodeCompiler::visit(const Codeparser::GroupingExpr& expr) { m_logger.debug("BytecodeCompiler: Visiting GroupingExpr."); expr.expression->accept(*this); }

        // --- Bytecode Emission Helpers ---

        void BytecodeCompiler::emitByte(uint8_t byte) {
            m_logger.debug("BytecodeCompiler: Emitting byte: " + std::to_string(byte));
            m_chunk.code.push_back(byte);
        }

        void BytecodeCompiler::emitBytes(uint8_t byte1, uint8_t byte2) {
            m_logger.debug("BytecodeCompiler: Emitting bytes: " + std::to_string(byte1) + ", " + std::to_string(byte2));
            emitByte(byte1);
            emitByte(byte2);
        }

        uint8_t BytecodeCompiler::makeConstant(const std::string& value) {
            m_logger.debug("BytecodeCompiler: Making constant: " + value);
            auto it = std::find(m_chunk.constants.begin(), m_chunk.constants.end(), value);
            if (it != m_chunk.constants.end()) {
                return static_cast<uint8_t>(std::distance(m_chunk.constants.begin(), it));
            }
            if (m_chunk.constants.size() >= 256) {
                throw BytecodeCompilerError("Too many constants in one chunk.");
            }
            m_chunk.constants.push_back(value);
            return static_cast<uint8_t>(m_chunk.constants.size() - 1);
        }

        uint8_t BytecodeCompiler::makeExternalReference(const std::string& signature) {
            m_logger.debug("BytecodeCompiler: Making external reference: " + signature);
            auto it = std::find(m_chunk.external_references.begin(), m_chunk.external_references.end(), signature);
            if (it != m_chunk.external_references.end()) {
                return static_cast<uint8_t>(std::distance(m_chunk.external_references.begin(), it));
            }
            if (m_chunk.external_references.size() >= 256) {
                throw BytecodeCompilerError("Too many external references in one chunk.");
            }
            m_chunk.external_references.push_back(signature);
            return static_cast<uint8_t>(m_chunk.external_references.size() - 1);
        }

        std::string BytecodeCompiler::getObfuscatedName(const std::string& original_name) {
            if (!m_obfuscate_enabled) {
                return original_name;
            }
            auto it = m_obfuscation_map.find(original_name);
            if (it != m_obfuscation_map.end()) {
                return it->second;
            }
            std::string obfuscated_name = "_o" + std::to_string(m_obfuscation_counter++);
            m_obfuscation_map[original_name] = obfuscated_name;
            m_logger.debug("Obfuscating '" + original_name + "' to '" + obfuscated_name + "'.");
            return obfuscated_name;
        }

    }
}
