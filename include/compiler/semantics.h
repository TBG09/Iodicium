#ifndef IODICIUM_COMPILER_SEMANTICS_H
#define IODICIUM_COMPILER_SEMANTICS_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include "codeparser/ast.h"
#include "common/logger.h"
#include "common/error.h"

// Forward declare to break circular dependency
namespace Iodicium {
    namespace Codeparser {
        class Lexer;
        class Parser;
    }
}

namespace Iodicium {
    namespace Compiler {

        enum class DataType {
            UNKNOWN,
            NIL,
            BOOL,
            INT,
            DOUBLE,
            STRING,
            FUNCTION
        };

        struct Symbol {
            DataType type;
            bool is_mutable;
            bool is_exported = false;
            bool is_external = false;
            int module_index = -1; // Index into the import table
        };

        class SymbolTable {
        private:
            std::map<std::string, Symbol> m_symbols;
        public:
            bool define(const std::string& name, const Symbol& symbol);
            Symbol* find(const std::string& name);
            // Added for compiler access
            const std::map<std::string, Symbol>& getSymbols() const { return m_symbols; }
        };

        class SemanticError : public Common::IodiciumError {
        public:
            SemanticError(const std::string& message, int line = -1, int column = -1)
                : Common::IodiciumError(message, line, column) {}
        };

        class SemanticAnalyzer : public Codeparser::StmtVisitor, public Codeparser::ExprVisitor {
        public:
            explicit SemanticAnalyzer(Common::Logger& logger, std::string base_path);
            void analyze(const std::vector<std::unique_ptr<Codeparser::Stmt>>& statements);

            // Getters for compiler access
            SymbolTable& getSymbolTable() { return m_symbol_table; }
            const std::vector<std::string>& getImportedModules() const { return m_imported_modules; }

            // Visitor methods for statements
            void visit(const Codeparser::ImportStmt& stmt) override;
            void visit(const Codeparser::VarStmt& stmt) override;
            void visit(const Codeparser::ExprStmt& stmt) override;
            void visit(const Codeparser::FunctionStmt& stmt) override;
            void visit(const Codeparser::ReturnStmt& stmt) override;

            // Visitor methods for expressions
            void visit(const Codeparser::AssignExpr& expr) override;
            void visit(const Codeparser::BinaryExpr& expr) override;
            void visit(const Codeparser::LiteralExpr& expr) override;
            void visit(const Codeparser::VariableExpr& expr) override;
            void visit(const Codeparser::GroupingExpr& expr) override;
            void visit(const Codeparser::CallExpr& expr) override;

        private:
            Common::Logger& m_logger;
            SymbolTable m_symbol_table;
            std::string m_base_path;
            DataType m_current_expr_type = DataType::UNKNOWN;
            std::vector<std::string> m_imported_modules;
            std::set<std::string> m_processed_imports; // For preventing circular imports

            void resolve(const std::unique_ptr<Codeparser::Stmt>& stmt);
            void resolve(const std::unique_ptr<Codeparser::Expr>& expr);
            DataType typeOf(const std::unique_ptr<Codeparser::Expr>& expr);
            DataType stringToDataType(const std::string& type_str);
        };

    }
}

#endif //IODICIUM_COMPILER_SEMANTICS_H
