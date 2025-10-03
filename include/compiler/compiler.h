#ifndef IODICIUM_COMPILER_COMPILER_H
#define IODICIUM_COMPILER_COMPILER_H

#include <vector>
#include <memory>
#include <string>
#include <map>
#include "codeparser/ast.h"
#include "common/logger.h" // Include logger header
#include "common/error.h"   // Include the base error class

namespace Iodicium {
    namespace Compiler {

        // Custom exception for semantic errors
        class SemanticError : public Common::IodiciumError {
        public:
            SemanticError(const std::string& message, int line, int column)
                : Common::IodiciumError(message, line, column) {}
        };

        // Represents a variable in the symbol table
        struct Symbol {
            Codeparser::Token declaration;
            bool is_mutable;
        };

        class Compiler : public Codeparser::StmtVisitor, public Codeparser::ExprVisitor {
        public:
            explicit Compiler(Common::Logger& logger); // Added logger parameter
            void compile(const std::vector<std::unique_ptr<Codeparser::Stmt>>& statements);

        private:
            Common::Logger& m_logger; // Added logger member
            std::map<std::string, Symbol> m_symbols;

            // Statement visitors
            void visit(const Codeparser::FunctionStmt& stmt) override;
            void visit(const Codeparser::ReturnStmt& stmt) override;
            void visit(const Codeparser::ExprStmt& stmt) override;
            void visit(const Codeparser::VarStmt& stmt) override;

            // Expression visitors
            void visit(const Codeparser::BinaryExpr& expr) override;
            void visit(const Codeparser::GroupingExpr& expr) override;
            void visit(const Codeparser::LiteralExpr& expr) override;
            void visit(const Codeparser::VariableExpr& expr) override;
            void visit(const Codeparser::CallExpr& expr) override;
            void visit(const Codeparser::AssignExpr& expr) override;

            void execute(const Codeparser::Stmt& stmt);
            void evaluate(const Codeparser::Expr& expr);
        };

    }
}

#endif //IODICIUM_COMPILER_COMPILER_H
