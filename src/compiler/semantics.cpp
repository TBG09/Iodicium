#include "compiler/semantics.h"
#include "codeparser/lexer.h" // For parsing imported files
#include "codeparser/parser.h" // For parsing imported files
#include "executable/iodl_reader.h" // New: For reading .iodl files
#include <fstream>
#include <sstream>
#include <map>

namespace Iodicium {
    namespace Compiler {

        // Helper map for DataType to string conversion
        static const std::map<DataType, std::string> DATA_TYPE_STRINGS = {
            {DataType::UNKNOWN, "Unknown"},
            {DataType::NIL,     "Nil"},
            {DataType::BOOL,    "Bool"},
            {DataType::INT,     "Int"},
            {DataType::DOUBLE,  "Double"},
            {DataType::STRING,  "String"},
            {DataType::FUNCTION,"Function"}
        };

        std::string dataTypeToString(DataType type) {
            auto it = DATA_TYPE_STRINGS.find(type);
            if (it != DATA_TYPE_STRINGS.end()) {
                return it->second;
            }
            return "InvalidType";
        }

        // --- SymbolTable Implementation ---
        bool SymbolTable::define(const std::string& name, const Symbol& symbol) {
            if (m_symbols.count(name) > 0) {
                return false;
            }
            m_symbols[name] = symbol;
            return true;
        }

        Symbol* SymbolTable::find(const std::string& name) {
            auto it = m_symbols.find(name);
            if (it != m_symbols.end()) {
                return &it->second;
            }
            return nullptr;
        }

        // --- SemanticAnalyzer Implementation ---
        SemanticAnalyzer::SemanticAnalyzer(Common::Logger& logger, std::string base_path) : m_logger(logger), m_base_path(std::move(base_path)) {
            // Pre-populate the symbol table with built-in functions
            m_symbol_table.define("writeOut", {DataType::FUNCTION, false, false, false, -1});
            m_symbol_table.define("writeErr", {DataType::FUNCTION, false, false, false, -1});
            m_symbol_table.define("flush", {DataType::FUNCTION, false, false, false, -1});
        }

        void SemanticAnalyzer::analyze(const std::vector<std::unique_ptr<Codeparser::Stmt>>& statements) {
            for (const auto& statement : statements) {
                resolve(statement);
            }
        }

        void SemanticAnalyzer::resolve(const std::unique_ptr<Codeparser::Stmt>& stmt) {
            stmt->accept(*this);
        }

        void SemanticAnalyzer::resolve(const std::unique_ptr<Codeparser::Expr>& expr) {
            expr->accept(*this);
        }

        // --- Visitor Implementations ---

        void SemanticAnalyzer::visit(const Codeparser::ImportStmt& stmt) {
            std::string relative_path = stmt.path.lexeme;
            std::string full_path = m_base_path + "/" + relative_path;
            m_logger.info("Analyzing import statement for: " + full_path);

            if (m_processed_imports.count(full_path)) {
                return; // Avoid circular imports
            }
            m_processed_imports.insert(full_path);

            // Add the module to our list and get its index for the runtime
            int module_index = m_imported_modules.size();
            m_imported_modules.push_back(relative_path); // Use relative path for the import table

            // Check file extension to decide how to handle the import
            if (full_path.size() > 5 && full_path.substr(full_path.size() - 5) == ".iodl") {
                m_logger.debug("Importing from .iodl library.");
                Executable::IodlReader reader(m_logger);
                Executable::LibraryChunk lib_chunk = reader.readFromFile(full_path);

                for (const auto& [name, ip] : lib_chunk.exports) {
                    m_logger.debug("Importing symbol '" + name + "' from library '" + full_path + "'.");
                    Symbol external_symbol = {DataType::FUNCTION, false, true, true, module_index};
                    if (!m_symbol_table.define(name, external_symbol)) {
                        throw SemanticError("Symbol '" + name + "' is already defined in this scope, but is also exported by '" + full_path + "'.", stmt.path.line, stmt.path.column);
                    }
                }
            } else {
                // Fallback for old .iodc import for now, can be removed later
                m_logger.debug("Importing from .iodc source file.");
                std::ifstream file(full_path);
                if (!file.is_open()) {
                    throw SemanticError("Could not open imported file: " + full_path, stmt.path.line, stmt.path.column);
                }
                std::stringstream buffer; buffer << file.rdbuf(); std::string source = buffer.str();
                Codeparser::Lexer import_lexer(source, m_logger);
                auto import_tokens = import_lexer.tokenize();
                Codeparser::Parser import_parser(import_tokens, m_logger);
                auto import_ast = import_parser.parse();
                std::string import_base_path = full_path.substr(0, full_path.find_last_of("/"));
                SemanticAnalyzer import_analyzer(m_logger, import_base_path);
                import_analyzer.analyze(import_ast);

                for (auto const& [name, symbol] : import_analyzer.m_symbol_table.getSymbols()) {
                    if (symbol.is_exported) {
                        Symbol external_symbol = symbol;
                        external_symbol.is_external = true;
                        external_symbol.module_index = module_index;
                        if (!m_symbol_table.define(name, external_symbol)) {
                            throw SemanticError("Symbol '" + name + "' is already defined in this scope, but is also exported by '" + full_path + "'.", stmt.path.line, stmt.path.column);
                        }
                    }
                }
            }
        }

        void SemanticAnalyzer::visit(const Codeparser::FunctionStmt& stmt) {
            Symbol symbol = {DataType::FUNCTION, false, stmt.is_exported};
            if (!m_symbol_table.define(stmt.name.lexeme, symbol)) {
                throw SemanticError("Symbol '" + stmt.name.lexeme + "' already declared in this scope.", stmt.name.line, stmt.name.column);
            }
            for (const auto& body_stmt : stmt.body) {
                resolve(body_stmt);
            }
        }

        void SemanticAnalyzer::visit(const Codeparser::VarStmt& stmt) {
            DataType declared_type = DataType::UNKNOWN;
            if (stmt.type_expr) {
                if (auto* type_var = dynamic_cast<Codeparser::VariableExpr*>(stmt.type_expr.get())) {
                    declared_type = stringToDataType(type_var->name.lexeme);
                    if (declared_type == DataType::UNKNOWN) {
                        throw SemanticError("Unknown type '" + type_var->name.lexeme + "'.", type_var->name.line, type_var->name.column);
                    }
                } else {
                    throw SemanticError("Invalid type expression.", stmt.name.line, stmt.name.column);
                }
            }
            DataType initializer_type = DataType::UNKNOWN;
            if (stmt.initializer) {
                initializer_type = typeOf(stmt.initializer);
            }
            DataType final_type = declared_type;
            if (final_type == DataType::UNKNOWN) {
                final_type = initializer_type;
            }
            if (final_type == DataType::UNKNOWN) {
                throw SemanticError("Cannot determine type for variable '" + stmt.name.lexeme + "'. Provide a type annotation or an initializer.", stmt.name.line, stmt.name.column);
            }
            if (declared_type != DataType::UNKNOWN && initializer_type != DataType::UNKNOWN && declared_type != initializer_type) {
                throw SemanticError("Initializer type '" + dataTypeToString(initializer_type) + "' does not match declared type '" + dataTypeToString(declared_type) + "' for variable '" + stmt.name.lexeme + "'.", stmt.name.line, stmt.name.column);
            }
            Symbol symbol = {final_type, stmt.is_mutable, stmt.is_exported};
            if (!m_symbol_table.define(stmt.name.lexeme, symbol)) {
                throw SemanticError("Variable '" + stmt.name.lexeme + "' already declared in this scope.", stmt.name.line, stmt.name.column);
            }
        }

        void SemanticAnalyzer::visit(const Codeparser::ExprStmt& stmt) { resolve(stmt.expression); }
        void SemanticAnalyzer::visit(const Codeparser::ReturnStmt& stmt) { if (stmt.value) { resolve(stmt.value); } }

        void SemanticAnalyzer::visit(const Codeparser::AssignExpr& expr) {
            Symbol* symbol = m_symbol_table.find(expr.name.lexeme);
            if (!symbol) {
                throw SemanticError("Undefined variable '" + expr.name.lexeme + "'.", expr.name.line, expr.name.column);
            }
            if (!symbol->is_mutable) {
                throw SemanticError("Cannot assign to immutable variable '" + expr.name.lexeme + "'.", expr.name.line, expr.name.column);
            }
            DataType value_type = typeOf(expr.value);
            if (symbol->type != value_type) {
                throw SemanticError("Cannot assign value of type '" + dataTypeToString(value_type) + "' to variable '" + expr.name.lexeme + "' of type '" + dataTypeToString(symbol->type) + "'.", expr.name.line, expr.name.column);
            }
            m_current_expr_type = value_type;
        }

        void SemanticAnalyzer::visit(const Codeparser::BinaryExpr& expr) {
            DataType left_type = typeOf(expr.left);
            DataType right_type = typeOf(expr.right);
            switch (expr.op.type) {
                case Codeparser::TokenType::PLUS:
                    if (left_type == DataType::STRING && right_type == DataType::STRING) { m_current_expr_type = DataType::STRING; }
                    else if (left_type == DataType::DOUBLE && right_type == DataType::DOUBLE) { m_current_expr_type = DataType::DOUBLE; }
                    else { throw SemanticError("Operator '+' cannot be applied to operands of type '" + dataTypeToString(left_type) + "' and '" + dataTypeToString(right_type) + "'.", expr.op.line, expr.op.column); }
                    break;
                case Codeparser::TokenType::MINUS: case Codeparser::TokenType::STAR: case Codeparser::TokenType::SLASH:
                    if (left_type == DataType::DOUBLE && right_type == DataType::DOUBLE) { m_current_expr_type = DataType::DOUBLE; }
                    else { throw SemanticError("Operator '" + expr.op.lexeme + "' cannot be applied to operands of type '" + dataTypeToString(left_type) + "' and '" + dataTypeToString(right_type) + "'.", expr.op.line, expr.op.column); }
                    break;
                default: throw SemanticError("Unsupported binary operator.", expr.op.line, expr.op.column);
            }
        }

        void SemanticAnalyzer::visit(const Codeparser::LiteralExpr& expr) {
            if (expr.token.type == Codeparser::TokenType::STRING_LITERAL) { m_current_expr_type = DataType::STRING; }
            else if (expr.token.type == Codeparser::TokenType::NUMBER_LITERAL) { m_current_expr_type = DataType::DOUBLE; }
        }

        void SemanticAnalyzer::visit(const Codeparser::VariableExpr& expr) {
            Symbol* symbol = m_symbol_table.find(expr.name.lexeme);
            if (!symbol) { throw SemanticError("Undefined variable '" + expr.name.lexeme + "'.", expr.name.line, expr.name.column); }
            m_current_expr_type = symbol->type;
        }

        void SemanticAnalyzer::visit(const Codeparser::GroupingExpr& expr) { resolve(expr.expression); }
        
        void SemanticAnalyzer::visit(const Codeparser::CallExpr& expr) {
            if (auto* callee = dynamic_cast<Codeparser::VariableExpr*>(expr.callee.get())) {
                Symbol* symbol = m_symbol_table.find(callee->name.lexeme);
                if (!symbol) { throw SemanticError("Undefined function '" + callee->name.lexeme + "'.", callee->name.line, callee->name.column); }
                if (symbol->type != DataType::FUNCTION) { throw SemanticError("'" + callee->name.lexeme + "' is not a function.", callee->name.line, callee->name.column); }
                m_current_expr_type = DataType::NIL; // Assume functions return Nil for now
            } else {
                throw SemanticError("Invalid callee expression.", expr.token.line, expr.token.column);
            }
        }

        DataType SemanticAnalyzer::typeOf(const std::unique_ptr<Codeparser::Expr>& expr) {
            resolve(expr);
            return m_current_expr_type;
        }

        DataType SemanticAnalyzer::stringToDataType(const std::string& type_str) {
            if (type_str == "String") return DataType::STRING;
            if (type_str == "Int") return DataType::INT;
            if (type_str == "Double") return DataType::DOUBLE;
            if (type_str == "Bool") return DataType::BOOL;
            if (type_str == "Function") return DataType::FUNCTION;
            return DataType::UNKNOWN;
        }
    }
}
