#include "compiler/compiler.h"
#include <stdexcept>

namespace Iodicium {
    namespace Compiler {

        Compiler::Compiler(Common::Logger& logger) : m_logger(logger) {
            m_logger.debug("Compiler constructor called.");
        }

        void Compiler::compile(const std::vector<std::unique_ptr<Codeparser::Stmt>>& statements) {
            m_logger.debug("Compiler: Starting compilation.");
            m_symbols.clear();
            for (const auto& statement : statements) {
                execute(*statement);
            }
            m_logger.debug("Compiler: Compilation finished.");
        }

        void Compiler::execute(const Codeparser::Stmt& stmt) {
            m_logger.debug("Compiler: Executing statement.");
            stmt.accept(*this);
        }

        void Compiler::evaluate(const Codeparser::Expr& expr) {
            m_logger.debug("Compiler: Evaluating expression.");
            expr.accept(*this);
        }

        // --- Statement Visitors ---

        void Compiler::visit(const Codeparser::VarStmt& stmt) {
            m_logger.debug("Compiler: Visiting VarStmt for variable '" + stmt.name.lexeme + "'.");
            if (m_symbols.count(stmt.name.lexeme)) {
                throw SemanticError("Variable '" + stmt.name.lexeme + "' already declared.", stmt.name.line, stmt.name.column);
            }
            m_symbols[stmt.name.lexeme] = {stmt.name, stmt.is_mutable};
            if (stmt.initializer) {
                m_logger.debug("Compiler: Evaluating initializer for '" + stmt.name.lexeme + "'.");
                evaluate(*stmt.initializer);
            }
        }

        void Compiler::visit(const Codeparser::FunctionStmt& stmt) {
            m_logger.debug("Compiler: Visiting FunctionStmt for function '" + stmt.name.lexeme + "'.");
            // For now, just execute the body statements. Full semantic analysis for functions would be more complex.
            for (const auto& statement : stmt.body) {
                execute(*statement);
            }
        }

        void Compiler::visit(const Codeparser::ReturnStmt& stmt) {
            m_logger.debug("Compiler: Visiting ReturnStmt.");
            if (stmt.value) {
                evaluate(*stmt.value);
            }
        }

        void Compiler::visit(const Codeparser::ExprStmt& stmt) {
            m_logger.debug("Compiler: Visiting ExprStmt.");
            evaluate(*stmt.expression);
        }

        // --- Expression Visitors ---

        void Compiler::visit(const Codeparser::AssignExpr& expr) {
            m_logger.debug("Compiler: Visiting AssignExpr for variable '" + expr.name.lexeme + "'.");
            auto symbol_it = m_symbols.find(expr.name.lexeme);
            if (symbol_it == m_symbols.end()) {
                throw SemanticError("Assignment to undeclared variable '" + expr.name.lexeme + "'.", expr.name.line, expr.name.column);
            }
            if (!symbol_it->second.is_mutable) {
                throw SemanticError("Cannot assign to immutable variable '" + expr.name.lexeme + "'.", expr.name.line, expr.name.column);
            }
            evaluate(*expr.value);
        }

        void Compiler::visit(const Codeparser::VariableExpr& expr) {
            m_logger.debug("Compiler: Visiting VariableExpr for variable '" + expr.name.lexeme + "'.");
            if (m_symbols.find(expr.name.lexeme) == m_symbols.end()) {
                // Allow 'print' and 'println' as built-in functions for now without declaration
                if (expr.name.lexeme != "print" && expr.name.lexeme != "println") {
                    throw SemanticError("Use of undeclared variable '" + expr.name.lexeme + "'.", expr.name.line, expr.name.column);
                }
            }
        }

        void Compiler::visit(const Codeparser::BinaryExpr& expr) {
            m_logger.debug("Compiler: Visiting BinaryExpr (operator: " + expr.op.lexeme + ").");
            evaluate(*expr.left);
            evaluate(*expr.right);
        }

        void Compiler::visit(const Codeparser::GroupingExpr& expr) {
            m_logger.debug("Compiler: Visiting GroupingExpr.");
            evaluate(*expr.expression);
        }

        void Compiler::visit(const Codeparser::LiteralExpr& expr) {
            m_logger.debug("Compiler: Visiting LiteralExpr (value: " + expr.value.lexeme + ").");
        }

        void Compiler::visit(const Codeparser::CallExpr& expr) {
            m_logger.debug("Compiler: Visiting CallExpr.");
            evaluate(*expr.callee);
            for (const auto& arg : expr.arguments) {
                evaluate(*arg);
            }
        }

    }
}
