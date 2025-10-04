#include "codeparser/parser.h"
#include "codeparser/ast.h"
#include <stdexcept>

namespace Iodicium {
    namespace Codeparser {

        Parser::Parser(std::vector<Token> tokens, Common::Logger& logger) : m_tokens(std::move(tokens)), m_logger(logger) {}

        std::vector<std::unique_ptr<Stmt>> Parser::parse() {
            std::vector<std::unique_ptr<Stmt>> statements;
            while (!is_at_end()) {

                while(peek().type == TokenType::NEWLINE) {
                    advance();
                }
                if (is_at_end()) break;

                auto stmt = parse_statement();
                if (stmt) {
                    statements.push_back(std::move(stmt));
                }
            }
            return statements;
        }

        std::unique_ptr<Stmt> Parser::parse_statement() {
            if (peek().type == TokenType::HASH) {
                advance();
                if (check(TokenType::IDENTIFIER) && peek().lexeme == "import") {
                    advance();
                    return parse_import_statement();
                } else {
                    while (peek().type != TokenType::NEWLINE && !is_at_end()) advance();
                    return nullptr;
                }
            }

            bool is_exported = false;
            if (match({TokenType::AT})) {
                Token annotation = consume(TokenType::IDENTIFIER, "Expect annotation name after '@'.");
                if (annotation.lexeme == "export") is_exported = true;
                else if (annotation.lexeme == "exportall") m_export_all = true;
                else error(annotation, "Unknown annotation.");


                while (peek().type == TokenType::NEWLINE) {
                    advance();
                }
            }

            if (peek().type == TokenType::DEF) {
                advance();
                return parse_function_statement(is_exported);
            }
            if (match({TokenType::VAL, TokenType::VAR})) {
                return parse_variable_declaration(is_exported);
            }

            if (is_exported) error(peek(), "Expect function or variable declaration after @export.");
            if (match({TokenType::RETURN})) return parse_return_statement();
            return parse_expression_statement();
        }

        std::unique_ptr<Stmt> Parser::parse_import_statement() {
            Token path = consume(TokenType::STRING_LITERAL, "Invalid import path format. Expected a quoted string.");
            return std::make_unique<ImportStmt>(path);
        }

        std::unique_ptr<Stmt> Parser::parse_variable_declaration(bool is_exported) {
            Token keyword = previous();
            bool is_mutable = (keyword.type == TokenType::VAR);
            Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");

            std::unique_ptr<Expr> type_expr = nullptr;
            if (match({TokenType::COLON})) {
                Token type_name = consume(TokenType::IDENTIFIER, "Expect type name after ':'.");
                type_expr = std::make_unique<VariableExpr>(type_name);
            }

            std::unique_ptr<Expr> initializer = nullptr;
            if (match({TokenType::EQUAL})) initializer = parse_expression();
            
            return std::make_unique<VarStmt>(name, std::move(type_expr), std::move(initializer), is_mutable, is_exported || m_export_all);
        }

        std::unique_ptr<Stmt> Parser::parse_function_statement(bool is_exported) {
            Token name = consume(TokenType::IDENTIFIER, "Expect function name.");
            consume(TokenType::LEFT_PAREN, "Expect '(' after function name.");
            
            std::vector<Parameter> parameters;
            if (!check(TokenType::RIGHT_PAREN)) {
                do {
                    Token param_name = consume(TokenType::IDENTIFIER, "Expect parameter name.");
                    std::unique_ptr<Expr> type_expr = nullptr;
                    if (match({TokenType::COLON})) {
                        Token type_name = consume(TokenType::IDENTIFIER, "Expect type name after ':'.");
                        type_expr = std::make_unique<VariableExpr>(type_name);
                    }
                    parameters.push_back({param_name, std::move(type_expr)});
                } while (match({TokenType::COMMA}));
            }
            
            consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");

            std::unique_ptr<Expr> return_type_expr = nullptr;
            if (match({TokenType::COLON})) {
                Token type_name = consume(TokenType::IDENTIFIER, "Expect return type name.");
                return_type_expr = std::make_unique<VariableExpr>(type_name);
            }
            
            if (match({TokenType::LEFT_BRACE})) {
                bool parent_export_all = m_export_all;
                m_export_all = false;
                std::vector<std::unique_ptr<Stmt>> body;
                while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {

                    while (peek().type == TokenType::NEWLINE) {
                        advance();
                    }


                    if (check(TokenType::RIGHT_BRACE) || is_at_end()) {
                        break;
                    }

                    auto stmt = parse_statement();
                    if (stmt) {
                        body.push_back(std::move(stmt));
                    }
                }
                consume(TokenType::RIGHT_BRACE, "Expect '}' after function body.");
                m_export_all = parent_export_all;
                return std::make_unique<FunctionStmt>(name, std::move(parameters), std::move(return_type_expr), std::move(body), is_exported || parent_export_all);
            } else {
                return std::make_unique<FunctionDeclStmt>(name, std::move(parameters), std::move(return_type_expr), is_exported || m_export_all);
            }
        }

        std::unique_ptr<Stmt> Parser::parse_return_statement() {
            std::unique_ptr<Expr> value = parse_expression();
            return std::make_unique<ReturnStmt>(previous(), std::move(value));
        }

        std::unique_ptr<Stmt> Parser::parse_expression_statement() {
            std::unique_ptr<Expr> expr = parse_expression();
            if (!check(TokenType::NEWLINE) && !check(TokenType::RIGHT_BRACE) && !is_at_end()) {
                error(peek(), "Expected newline after expression.");
            }
            return std::make_unique<ExprStmt>(std::move(expr));
        }

        std::unique_ptr<Expr> Parser::parse_expression() { return parse_assignment(); }

        std::unique_ptr<Expr> Parser::parse_assignment() {
            std::unique_ptr<Expr> expr = parse_term();
            if (match({TokenType::EQUAL})) {
                Token equals = previous();
                std::unique_ptr<Expr> value = parse_assignment();
                if (auto* var = dynamic_cast<VariableExpr*>(expr.get())) {
                    return std::make_unique<AssignExpr>(var->name, std::move(value));
                }
                error(equals, "Invalid assignment target.");
            }
            return expr;
        }

        std::unique_ptr<Expr> Parser::parse_term() {
            std::unique_ptr<Expr> expr = parse_factor();
            while (match({TokenType::MINUS, TokenType::PLUS})) {
                Token op = previous();
                std::unique_ptr<Expr> right = parse_factor();
                expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
            }
            return expr;
        }

        std::unique_ptr<Expr> Parser::parse_factor() {
            std::unique_ptr<Expr> expr = parse_unary();
            while (match({TokenType::SLASH, TokenType::STAR})) {
                Token op = previous();
                std::unique_ptr<Expr> right = parse_unary();
                expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
            }
            return expr;
        }

        std::unique_ptr<Expr> Parser::parse_unary() {
            if (match({TokenType::MINUS})) { /* Simplified */ }            return parse_call();
        }

        std::unique_ptr<Expr> Parser::parse_call() {
            std::unique_ptr<Expr> expr = parse_primary();
            if (match({TokenType::LEFT_PAREN})) {
                std::vector<std::unique_ptr<Expr>> arguments;
                if (!check(TokenType::RIGHT_PAREN)) {
                    do {
                        arguments.push_back(parse_expression());
                    } while (match({TokenType::COMMA}));
                }
                Token paren = consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
                return std::make_unique<CallExpr>(std::move(expr), paren, std::move(arguments));
            }
            return expr;
        }

        std::unique_ptr<Expr> Parser::parse_primary() {
            if (match({TokenType::STRING_LITERAL, TokenType::NUMBER_LITERAL})) return std::make_unique<LiteralExpr>(previous());
            if (match({TokenType::IDENTIFIER})) return std::make_unique<VariableExpr>(previous());
            if (match({TokenType::LEFT_PAREN})) {
                auto expr = parse_expression();
                consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
                return std::make_unique<GroupingExpr>(previous(), std::move(expr));
            }
            throw Common::UnexpectedTokenException("expression", peek());
        }

        bool Parser::is_at_end() { return m_current >= m_tokens.size() || peek().type == TokenType::END_OF_FILE; }
        
        Token& Parser::peek() {
            if (m_current >= m_tokens.size()) {
                throw std::out_of_range("Attempted to peek past end of tokens.");
            }
            return m_tokens[m_current];
        }

        Token& Parser::previous() {
            if (m_current == 0 || m_current > m_tokens.size()) {
                throw std::out_of_range("Attempted to get previous token at invalid index.");
            }
            return m_tokens[m_current - 1];
        }

        Token& Parser::advance() { if (!is_at_end()) m_current++; return previous(); }
        bool Parser::check(TokenType type) { if (is_at_end()) return false; return peek().type == type; }

        bool Parser::match(const std::vector<TokenType>& types) {
            for (TokenType type : types) {
                if (check(type)) {
                    advance();
                    return true;
                }
            }            return false;
        }

        Token& Parser::consume(TokenType type, const std::string& message) {
            if (check(type)) return advance();
            if (is_at_end()) {
                if (!m_tokens.empty()) {
                    error(previous(), message + " (Unexpected end of file)");
                } else {
                    throw ParserError(message + " (Empty file)", 0, 0);
                }
            } else {
                error(peek(), message);
            }
            throw std::runtime_error("Unreachable");
        }

        void Parser::error(const Token& token, const std::string& message) {
            throw ParserError(message, token.line, token.column);
        }

    }
}
