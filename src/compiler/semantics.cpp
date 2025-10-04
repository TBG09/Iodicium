#include "compiler/semantics.h"
#include "codeparser/lexer.h"
#include "codeparser/parser.h"
#include <fstream>
#include <sstream>
#include <map>

namespace Iodicium {
    namespace Compiler {

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
            if (it != DATA_TYPE_STRINGS.end()) return it->second;
            return "InvalidType";
        }

        SymbolTable::SymbolTable(Common::Logger& logger) : m_logger(logger) {
            beginScope();
        }

        void SymbolTable::beginScope() {
            m_logger.debug("[SymbolTable] Beginning new scope.");
            m_scopes.emplace_back();
        }

        void SymbolTable::endScope() {
            m_logger.debug("[SymbolTable] Ending current scope.");
            if (!m_scopes.empty()) m_scopes.pop_back();
        }

        bool SymbolTable::define(const std::string& name, const Symbol& symbol) {
            if (m_scopes.empty()) return false;
            m_logger.debug("[SymbolTable] Defining symbol '" + name + "' in current scope.");
            if (m_scopes.back().count(name) > 0) {
                m_logger.debug("[SymbolTable] Symbol '" + name + "' already exists in current scope.");
                return false;
            }
            m_scopes.back()[name] = symbol;
            return true;
        }

        Symbol* SymbolTable::find(const std::string& name) {
            m_logger.debug("[SymbolTable] Searching for symbol '" + name + "'.");
            for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
                auto symbol_it = it->find(name);
                if (symbol_it != it->end()) {
                    m_logger.debug("[SymbolTable] Found symbol '" + name + "'.");
                    return &symbol_it->second;
                }
            }
            m_logger.debug("[SymbolTable] Symbol '" + name + "' not found.");
            return nullptr;
        }

        SemanticAnalyzer::SemanticAnalyzer(Common::Logger& logger, std::string base_path) : m_logger(logger), m_base_path(std::move(base_path)), m_symbol_table(logger) {
            m_logger.debug("[SemanticAnalyzer] Defining built-in functions...");
            m_symbol_table.define("writeOut", {DataType::FUNCTION, DataType::NIL, false, false, false, -1});
            m_symbol_table.define("writeErr", {DataType::FUNCTION, DataType::NIL, false, false, false, -1});
            m_symbol_table.define("flush", {DataType::FUNCTION, DataType::NIL, false, false, false, -1});
            m_symbol_table.define("convert", {DataType::FUNCTION, DataType::UNKNOWN, false, false, false, -1});
            m_logger.debug("[SemanticAnalyzer] Built-in functions defined.");
        }

        void SemanticAnalyzer::analyze(const std::vector<std::unique_ptr<Codeparser::Stmt>>& statements) {
            for (const auto& statement : statements) {
                resolve(statement);
            }
        }

        void SemanticAnalyzer::resolve(const std::unique_ptr<Codeparser::Stmt>& stmt) {
            if (!stmt) return;
            stmt->accept(*this);
        }

        void SemanticAnalyzer::resolve(const std::unique_ptr<Codeparser::Expr>& expr) {
            if (!expr) { m_current_expr_type = DataType::UNKNOWN; return; }
            expr->accept(*this);
        }

        void SemanticAnalyzer::visit(const Codeparser::ImportStmt& stmt) {
            std::string relative_path = stmt.path.lexeme;
            if (relative_path.length() >= 2 && relative_path.front() == '\"' && relative_path.back() == '\"') {
                relative_path = relative_path.substr(1, relative_path.length() - 2);
            }

            size_t dot_pos = relative_path.find_last_of('.');
            if (dot_pos == std::string::npos || (relative_path.substr(dot_pos) != ".iodc" && relative_path.substr(dot_pos) != ".iodl")) {
                relative_path += ".iodc";
            }

            std::string full_path = m_base_path + "/" + relative_path;
            if (m_processed_imports.count(full_path)) {
                m_logger.debug("[SemanticAnalyzer] Already processed import: " + full_path);
                return;
            }
            m_processed_imports.insert(full_path);

            std::ifstream file(full_path);
            if (!file.is_open()) throw SemanticError("Could not open imported file: " + full_path, stmt.path.line, stmt.path.column);
            std::stringstream buffer; buffer << file.rdbuf();
            std::string source = buffer.str();

            Codeparser::Lexer imported_lexer(source, m_logger);
            auto imported_tokens = imported_lexer.tokenize();
            Codeparser::Parser imported_parser(imported_tokens, m_logger);
            auto imported_ast = imported_parser.parse();

            m_logger.debug("[SemanticAnalyzer] Analyzing imported file: " + full_path);
            bool previous_import_state = m_is_importing;
            m_is_importing = true;
            for (const auto& imported_stmt : imported_ast) {
                resolve(imported_stmt);
            }
            m_is_importing = previous_import_state;
            m_logger.debug("[SemanticAnalyzer] Finished analyzing imported file: " + full_path);
        }

        void SemanticAnalyzer::visit(const Codeparser::FunctionStmt& stmt) {
            if (m_is_importing && !stmt.is_exported) {
                return;
            }

            DataType return_type = DataType::NIL;
            if (stmt.return_type_expr) {
                if (auto* type_var = dynamic_cast<Codeparser::VariableExpr*>(stmt.return_type_expr.get())) {
                    return_type = stringToDataType(type_var->name.lexeme);
                }
            }
            Symbol func_symbol = {DataType::FUNCTION, return_type, false, stmt.is_exported, false, -1};
            if (!m_symbol_table.define(stmt.name.lexeme, func_symbol)) {
                m_logger.warn("[SemanticAnalyzer] Ignoring re-declaration of function '" + stmt.name.lexeme + "'.");
                if (m_is_importing) return;
            }

            if (m_is_importing) {
                return;
            }

            m_symbol_table.beginScope();
            for (const auto& param : stmt.params) {
                DataType param_type = DataType::UNKNOWN;
                if (param.type_expr) {
                    if (auto* type_var = dynamic_cast<Codeparser::VariableExpr*>(param.type_expr.get())) {
                        param_type = stringToDataType(type_var->name.lexeme);
                    }
                }
                if (param_type == DataType::UNKNOWN) {
                    throw SemanticError("Parameter '" + param.name.lexeme + "' must have a type.", param.name.line, param.name.column);
                }
                m_symbol_table.define(param.name.lexeme, {param_type, DataType::NIL, false, false, false, -1});
            }

            m_logger.debug("[SemanticAnalyzer] Processing body of function: " + stmt.name.lexeme);
            for (const auto& body_stmt : stmt.body) {
                resolve(body_stmt);
            }

            m_symbol_table.endScope();
        }

        void SemanticAnalyzer::visit(const Codeparser::FunctionDeclStmt& stmt) {
            if (m_is_importing && !stmt.is_exported) return;

            DataType return_type = DataType::NIL;
            if (stmt.return_type_expr) {
                if (auto* type_var = dynamic_cast<Codeparser::VariableExpr*>(stmt.return_type_expr.get())) {
                    return_type = stringToDataType(type_var->name.lexeme);
                }
            }
            Symbol symbol = {DataType::FUNCTION, return_type, false, stmt.is_exported, false, -1};
            if (!m_symbol_table.define(stmt.name.lexeme, symbol)) {
                m_logger.warn("[SemanticAnalyzer] Ignoring re-declaration of function declaration '" + stmt.name.lexeme + "'.");
            }
        }

        void SemanticAnalyzer::visit(const Codeparser::VarStmt& stmt) {
            if (m_is_importing && !stmt.is_exported) return;

            DataType declared_type = DataType::UNKNOWN;
            if (stmt.type_expr) {
                if (auto* type_var = dynamic_cast<Codeparser::VariableExpr*>(stmt.type_expr.get())) {
                    declared_type = stringToDataType(type_var->name.lexeme);
                }
            }
            
            DataType initializer_type = typeOf(stmt.initializer);
            if (declared_type != DataType::UNKNOWN && initializer_type != DataType::UNKNOWN && declared_type != initializer_type) {
                throw SemanticError("Initializer type '" + dataTypeToString(initializer_type) + "' does not match declared type '" + dataTypeToString(declared_type) + "'.", stmt.name.line, stmt.name.column);
            }
            
            DataType final_type = (declared_type != DataType::UNKNOWN) ? declared_type : initializer_type;
            if (final_type == DataType::UNKNOWN) {
                throw SemanticError("Cannot determine type for variable '" + stmt.name.lexeme + "'.", stmt.name.line, stmt.name.column);
            }
            
            Symbol symbol = {final_type, DataType::NIL, stmt.is_mutable, stmt.is_exported, false, -1};
            if (!m_symbol_table.define(stmt.name.lexeme, symbol)) {
                throw SemanticError("Variable '" + stmt.name.lexeme + "' already declared in this scope.", stmt.name.line, stmt.name.column);
            }
        }

        void SemanticAnalyzer::visit(const Codeparser::VariableExpr& expr) {
            Symbol* symbol = m_symbol_table.find(expr.name.lexeme);
            if (!symbol) { throw SemanticError("Undefined variable '" + expr.name.lexeme + "'.", expr.name.line, expr.name.column); }
            m_current_expr_type = symbol->type;
        }

        void SemanticAnalyzer::visit(const Codeparser::AssignExpr& expr) {
            Symbol* symbol = m_symbol_table.find(expr.name.lexeme);
            if (!symbol) throw SemanticError("Undefined variable '" + expr.name.lexeme + "'.", expr.name.line, expr.name.column);
            if (!symbol->is_mutable) throw SemanticError("Cannot assign to immutable variable '" + expr.name.lexeme + "'.", expr.name.line, expr.name.column);
            DataType value_type = typeOf(expr.value);
            if (symbol->type != value_type) {
                throw SemanticError("Cannot assign value of type '" + dataTypeToString(value_type) + "' to variable '" + expr.name.lexeme + "' of type '" + dataTypeToString(symbol->type) + "'.", expr.name.line, expr.name.column);
            }
            m_current_expr_type = value_type;
        }

        void SemanticAnalyzer::visit(const Codeparser::BinaryExpr& expr) {
            DataType left_type = typeOf(expr.left);
            DataType right_type = typeOf(expr.right);

            if (expr.op.lexeme == "+") {
                if (left_type == DataType::STRING || right_type == DataType::STRING) {
                    m_current_expr_type = DataType::STRING;
                    return;
                }
            }

            if ((left_type == DataType::INT || left_type == DataType::DOUBLE) &&
                (right_type == DataType::INT || right_type == DataType::DOUBLE)) {
                if (expr.op.lexeme == "+" || expr.op.lexeme == "-" || expr.op.lexeme == "*" || expr.op.lexeme == "/") {
                    if (left_type == DataType::DOUBLE || right_type == DataType::DOUBLE) {
                        m_current_expr_type = DataType::DOUBLE;
                    } else {
                        m_current_expr_type = DataType::INT;
                    }
                    return;
                }
            }

            throw SemanticError("Operator '" + expr.op.lexeme + "' cannot be applied to operands of type '" + dataTypeToString(left_type) + "' and '" + dataTypeToString(right_type) + "'.", expr.op.line, expr.op.column);
        }

        void SemanticAnalyzer::visit(const Codeparser::CallExpr& expr) {
            if (auto* callee = dynamic_cast<Codeparser::VariableExpr*>(expr.callee.get())) {
                if (callee->name.lexeme == "convert") {
                    if (expr.arguments.size() != 2) {
                        throw SemanticError("convert() requires 2 arguments: the value and the target type.", callee->name.line, callee->name.column);
                    }
                    resolve(expr.arguments[0]);
                    auto* type_arg = dynamic_cast<Codeparser::VariableExpr*>(expr.arguments[1].get());
                    if (!type_arg) {
                        throw SemanticError("The second argument to convert() must be a type name (e.g., Int, String).", callee->name.line, callee->name.column);
                    }
                    DataType target_type = stringToDataType(type_arg->name.lexeme);
                    if (target_type == DataType::UNKNOWN) {
                        throw SemanticError("Unknown type '" + type_arg->name.lexeme + "' for conversion.", type_arg->name.line, type_arg->name.column);
                    }
                    m_current_expr_type = target_type;
                    return;
                }

                Symbol* symbol = m_symbol_table.find(callee->name.lexeme);
                if (!symbol) { throw SemanticError("Undefined function '" + callee->name.lexeme + "'.", callee->name.line, callee->name.column); }
                if (symbol->type != DataType::FUNCTION) { throw SemanticError("'" + callee->name.lexeme + "' is not a function.", callee->name.line, callee->name.column); }

                for (const auto& arg : expr.arguments) {
                    resolve(arg);
                }

                m_current_expr_type = symbol->return_type;
            } else {
                throw SemanticError("Invalid callee expression.", expr.token.line, expr.token.column);
            }
        }

        void SemanticAnalyzer::visit(const Codeparser::ExprStmt& stmt) { resolve(stmt.expression); }
        void SemanticAnalyzer::visit(const Codeparser::ReturnStmt& stmt) { if (stmt.value) { resolve(stmt.value); } }
        void SemanticAnalyzer::visit(const Codeparser::LiteralExpr& expr) { 
            if (expr.token.type == Codeparser::TokenType::STRING_LITERAL) {
                m_current_expr_type = DataType::STRING;
            } else if (expr.token.type == Codeparser::TokenType::NUMBER_LITERAL) {
                m_current_expr_type = DataType::DOUBLE;
            }
        }
        void SemanticAnalyzer::visit(const Codeparser::GroupingExpr& expr) { resolve(expr.expression); }

        DataType SemanticAnalyzer::typeOf(const std::unique_ptr<Codeparser::Expr>& expr) {
            if (!expr) return DataType::UNKNOWN;
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
