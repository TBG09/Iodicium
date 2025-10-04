#include "compiler/codegen.h"
#include <stdexcept>
#include <algorithm>

namespace Iodicium {
    namespace Compiler {

        using Executable::Chunk;

        static DataType stringToDataType(const std::string& type_str) {
            if (type_str == "String") return DataType::STRING;
            if (type_str == "Int") return DataType::INT;
            if (type_str == "Double") return DataType::DOUBLE;
            if (type_str == "Bool") return DataType::BOOL;
            return DataType::UNKNOWN;
        }

        BytecodeCompiler::BytecodeCompiler(Common::Logger& logger, SemanticAnalyzer& analyzer, bool obfuscate_enabled) 
            : m_logger(logger), m_analyzer(analyzer), m_obfuscate_enabled(obfuscate_enabled) {}

        Executable::Chunk BytecodeCompiler::compile(const std::vector<std::unique_ptr<Codeparser::Stmt>>& statements) {
            m_logger.debug("BytecodeCompiler: Starting compilation.");
            m_chunk = Executable::Chunk();
            m_function_ips.clear();
            m_call_fixups.clear();
            m_locals.clear();
            m_scope_depth = 0;

            for (const auto& statement : statements) {
                statement->accept(*this);
            }

            m_logger.debug("BytecodeCompiler: Starting backpatching pass.");
            for (const auto& [func_name, offsets] : m_call_fixups) {
                auto it = m_function_ips.find(func_name);
                if (it == m_function_ips.end()) {
                    throw BytecodeCompilerError("Internal Compiler Error: Undefined function '" + func_name + "' in fixup pass.");
                }
                size_t address = it->second;
                for (size_t offset : offsets) {
                    patchShort(offset, static_cast<uint16_t>(address));
                }
            }

            emitByte(OP_RETURN);
            m_logger.debug("BytecodeCompiler: Finished compilation.");
            return m_chunk;
        }

        void BytecodeCompiler::beginScope() { m_scope_depth++; }

        void BytecodeCompiler::endScope() {
            m_scope_depth--;
            while (!m_locals.empty() && m_locals.back().depth > m_scope_depth) {
                m_locals.pop_back();
            }
        }

        void BytecodeCompiler::visit(const Codeparser::FunctionStmt& stmt) {
            m_logger.debug("BytecodeCompiler: Defining function '" + stmt.name.lexeme + "'.");
            size_t function_ip = m_chunk.code.size();
            m_function_ips[stmt.name.lexeme] = function_ip;

            beginScope();

            for (const auto& param : stmt.params) {
                m_locals.push_back({param.name, m_scope_depth});
            }

            for (const auto& statement : stmt.body) {
                statement->accept(*this);
            }
            
            emitBytes(OP_CONST, makeConstant(""));
            emitByte(OP_RETURN);

            endScope();
        }

        void BytecodeCompiler::visit(const Codeparser::VarStmt& stmt) {
            if (stmt.initializer) {
                stmt.initializer->accept(*this);
            } else {
                emitBytes(OP_CONST, makeConstant(""));
            }

            if (m_scope_depth > 0) {
                m_locals.push_back({stmt.name, m_scope_depth});
                return;
            }

            uint8_t name_index = makeConstant(getObfuscatedName(stmt.name.lexeme));
            emitBytes(OP_DEFINE_GLOBAL, name_index);
        }

        int BytecodeCompiler::resolveLocal(const Codeparser::Token& name) {
            for (int i = m_locals.size() - 1; i >= 0; i--) {
                if (m_locals[i].name.lexeme == name.lexeme) {
                    return i;
                }
            }
            return -1;
        }

        void BytecodeCompiler::visit(const Codeparser::VariableExpr& expr) {
            int local_index = resolveLocal(expr.name);
            if (local_index != -1) {
                emitBytes(OP_GET_LOCAL, static_cast<uint8_t>(local_index));
            } else {
                uint8_t name_index = makeConstant(getObfuscatedName(expr.name.lexeme));
                emitBytes(OP_GET_GLOBAL, name_index);
            }
        }

        void BytecodeCompiler::visit(const Codeparser::AssignExpr& expr) {
            expr.value->accept(*this);
            int local_index = resolveLocal(expr.name);
            if (local_index != -1) {
                emitBytes(OP_SET_LOCAL, static_cast<uint8_t>(local_index));
            } else {
                uint8_t name_index = makeConstant(getObfuscatedName(expr.name.lexeme));
                emitBytes(OP_SET_GLOBAL, name_index);
            }
        }

        void BytecodeCompiler::visit(const Codeparser::CallExpr& expr) {
            if (auto* callee = dynamic_cast<Codeparser::VariableExpr*>(expr.callee.get())) {
                if (callee->name.lexeme == "writeOut" || callee->name.lexeme == "writeErr") {
                    expr.arguments[0]->accept(*this);
                    emitByte(callee->name.lexeme == "writeOut" ? OP_WRITE_OUT : OP_WRITE_ERR);
                    return;
                } else if (callee->name.lexeme == "flush") {
                    emitByte(OP_FLUSH);
                    return;
                } else if (callee->name.lexeme == "convert") {
                    expr.arguments[0]->accept(*this);
                    auto* type_arg = dynamic_cast<Codeparser::VariableExpr*>(expr.arguments[1].get());
                    if (!type_arg) { throw BytecodeCompilerError("Second arg to convert() must be a type.", expr.token.line, expr.token.column); }
                    emitBytes(OP_CONVERT, (uint8_t)stringToDataType(type_arg->name.lexeme));
                    return;
                }

                for (const auto& arg : expr.arguments) {
                    arg->accept(*this);
                }

                emitByte(OP_CALL);
                emitByte(static_cast<uint8_t>(expr.arguments.size()));

                auto it = m_function_ips.find(callee->name.lexeme);
                if (it != m_function_ips.end()) {
                    emitShort(static_cast<uint16_t>(it->second));
                } else {
                    size_t offset = m_chunk.code.size();
                    emitShort(0xFFFF);
                    m_call_fixups[callee->name.lexeme].push_back(offset);
                }
                return;
            }
            throw BytecodeCompilerError("Invalid callee expression.", expr.callee->token.line, expr.callee->token.column);
        }

        void BytecodeCompiler::visit(const Codeparser::ImportStmt& stmt) {}
        void BytecodeCompiler::visit(const Codeparser::FunctionDeclStmt& stmt) {}
        void BytecodeCompiler::visit(const Codeparser::ReturnStmt& stmt) { if (stmt.value) { stmt.value->accept(*this); } emitByte(OP_RETURN); }
        void BytecodeCompiler::visit(const Codeparser::ExprStmt& stmt) { stmt.expression->accept(*this); }
        void BytecodeCompiler::visit(const Codeparser::LiteralExpr& expr) { uint8_t const_index = makeConstant(expr.value.lexeme); emitBytes(OP_CONST, const_index); }
        void BytecodeCompiler::visit(const Codeparser::BinaryExpr& expr) { expr.left->accept(*this); expr.right->accept(*this); switch (expr.op.type) { case Codeparser::TokenType::PLUS: emitByte(OP_ADD); break; case Codeparser::TokenType::MINUS: emitByte(OP_SUBTRACT); break; case Codeparser::TokenType::STAR: emitByte(OP_MULTIPLY); break; case Codeparser::TokenType::SLASH: emitByte(OP_DIVIDE); break; default: throw BytecodeCompilerError("Unsupported binary operator.", expr.op.line, expr.op.column); } }
        void BytecodeCompiler::visit(const Codeparser::GroupingExpr& expr) { expr.expression->accept(*this); }

        void BytecodeCompiler::emitByte(uint8_t byte) { m_chunk.code.push_back(byte); }
        void BytecodeCompiler::emitBytes(uint8_t byte1, uint8_t byte2) { emitByte(byte1); emitByte(byte2); }
        void BytecodeCompiler::emitShort(uint16_t value) { emitByte((value >> 8) & 0xFF); emitByte(value & 0xFF); }
        void BytecodeCompiler::patchShort(size_t offset, uint16_t value) { m_chunk.code[offset] = (value >> 8) & 0xFF; m_chunk.code[offset + 1] = value & 0xFF; }
        uint8_t BytecodeCompiler::makeConstant(const std::string& value) { auto it = std::find(m_chunk.constants.begin(), m_chunk.constants.end(), value); if (it != m_chunk.constants.end()) { return static_cast<uint8_t>(std::distance(m_chunk.constants.begin(), it)); } if (m_chunk.constants.size() >= 256) { throw BytecodeCompilerError("Too many constants in one chunk."); } m_chunk.constants.push_back(value); return static_cast<uint8_t>(m_chunk.constants.size() - 1); }
        std::string BytecodeCompiler::getObfuscatedName(const std::string& original_name) { if (!m_obfuscate_enabled) { return original_name; } auto it = m_obfuscation_map.find(original_name); if (it != m_obfuscation_map.end()) { return it->second; } std::string obfuscated_name = "_o" + std::to_string(m_obfuscation_counter++); m_obfuscation_map[original_name] = obfuscated_name; return obfuscated_name; }

    }
}
