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

            // First pass: Compile functions and populate the function IP map
            for (const auto& statement : statements) {
                if (dynamic_cast<Codeparser::FunctionStmt*>(statement.get())) {
                    statement->accept(*this);
                }
            }

            // Second pass: Compile the rest of the code (main body)
            for (const auto& statement : statements) {
                if (!dynamic_cast<Codeparser::FunctionStmt*>(statement.get())) {
                    statement->accept(*this);
                }
            }

            emitByte(OP_RETURN);
            m_logger.debug("BytecodeCompiler: Finished bytecode compilation.");
            return m_chunk;
        }

        // --- Visitor Implementations ---

        void BytecodeCompiler::visit(const Codeparser::ImportStmt& stmt) {
            // Imports are handled by the linker/analyzer. The code generator can ignore them.
        }

        void BytecodeCompiler::visit(const Codeparser::FunctionStmt& stmt) {
            m_logger.debug("BytecodeCompiler: Compiling function '" + stmt.name.lexeme + "'.");
            size_t function_ip = m_chunk.code.size();
            m_function_ips[stmt.name.lexeme] = function_ip;

            for (const auto& statement : stmt.body) {
                statement->accept(*this);
            }
            emitByte(OP_RETURN);
        }

        void BytecodeCompiler::visit(const Codeparser::FunctionDeclStmt& stmt) {
            // A declaration produces no executable code. Do nothing.
        }

        void BytecodeCompiler::visit(const Codeparser::ReturnStmt& stmt) {
            if (stmt.value) {
                stmt.value->accept(*this);
            }
            emitByte(OP_RETURN);
        }

        void BytecodeCompiler::visit(const Codeparser::ExprStmt& stmt) {
            stmt.expression->accept(*this);
        }

        void BytecodeCompiler::visit(const Codeparser::LiteralExpr& expr) {
            uint8_t const_index = makeConstant(expr.value.lexeme);
            emitBytes(OP_CONST, const_index);
        }

        void BytecodeCompiler::visit(const Codeparser::CallExpr& expr) {
            if (auto* callee = dynamic_cast<Codeparser::VariableExpr*>(expr.callee.get())) {
                // Handle built-in functions
                if (callee->name.lexeme == "writeOut") {
                    expr.arguments[0]->accept(*this);
                    emitByte(OP_WRITE_OUT);
                    return;
                } else if (callee->name.lexeme == "writeErr") {
                    expr.arguments[0]->accept(*this);
                    emitByte(OP_WRITE_ERR);
                    return;
                } else if (callee->name.lexeme == "flush") {
                    emitByte(OP_FLUSH);
                    return;
                }

                // It's a user-defined function. Look it up in our function map.
                auto it = m_function_ips.find(callee->name.lexeme);
                if (it != m_function_ips.end()) {
                    m_logger.debug("BytecodeCompiler: Compiling static call to '" + callee->name.lexeme + "'.");
                    emitByte(OP_CALL);
                    emitShort(static_cast<uint16_t>(it->second)); // Emit the 16-bit address
                    return;
                }
            }
            throw BytecodeCompilerError("Cannot compile call to '" + expr.callee->token.lexeme + "'.", expr.callee->token.line, expr.callee->token.column);
        }

        void BytecodeCompiler::visit(const Codeparser::VarStmt& stmt) {
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
            std::string var_name = getObfuscatedName(expr.name.lexeme);
            uint8_t name_index = makeConstant(var_name);
            emitBytes(OP_GET_GLOBAL, name_index);
        }

        void BytecodeCompiler::visit(const Codeparser::AssignExpr& expr) {
            expr.value->accept(*this);
            std::string var_name = getObfuscatedName(expr.name.lexeme);
            uint8_t name_index = makeConstant(var_name);
            emitBytes(OP_SET_GLOBAL, name_index);
        }

        void BytecodeCompiler::visit(const Codeparser::BinaryExpr& expr) {
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

        void BytecodeCompiler::visit(const Codeparser::GroupingExpr& expr) { expr.expression->accept(*this); }

        // --- Bytecode Emission Helpers ---

        void BytecodeCompiler::emitByte(uint8_t byte) {
            m_chunk.code.push_back(byte);
        }

        void BytecodeCompiler::emitBytes(uint8_t byte1, uint8_t byte2) {
            emitByte(byte1);
            emitByte(byte2);
        }

        void BytecodeCompiler::emitShort(uint16_t value) {
            emitByte((value >> 8) & 0xFF);
            emitByte(value & 0xFF);
        }

        uint8_t BytecodeCompiler::makeConstant(const std::string& value) {
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
            return obfuscated_name;
        }

    }
}
