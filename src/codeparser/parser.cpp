#include "codeparser/parser.h"
#include <stdexcept>

namespace Iodicium {
    namespace Codeparser {

        Parser::Parser(std::vector<Token> tokens, Common::Logger& logger) : m_tokens(std::move(tokens)), m_logger(logger) {}

        std::vector<std::unique_ptr<Stmt>> Parser::parse() {
            std::vector<std::unique_ptr<Stmt>> statements;
            while (!is_at_end()) {
                auto stmt = parse_statement();
                if (stmt) { // Only add if a statement was actually produced
                    statements.push_back(std::move(stmt));
                }
            }
            return statements;
        }

        // --- Statement Parsing ---

        std::unique_ptr<Stmt> Parser::parse_statement() {
            m_logger.debug("Parsing statement, current token: " + peek().lexeme + " (Type: " + std::to_string(static_cast<int>(peek().type)) + ")");

            if (match({TokenType::HASH})) {
                return parse_import_statement();
            }

            bool is_exported = false;
            if (match({TokenType::AT})) {
                Token annotation = consume(TokenType::IDENTIFIER, "Expect annotation name after '@'.");
                if (annotation.lexeme == "export") {
                    is_exported = true;
                } else if (annotation.lexeme == "exportall") {
                    m_export_all = true;
                    return nullptr; // Directive, no statement produced.
                } else {
                    error(annotation, "Unknown annotation.");
                }
            }

            if (match({TokenType::VAL, TokenType::VAR})) {
                return parse_variable_declaration(is_exported);
            }
            if (match({TokenType::DEF})) {
                return parse_function_statement(is_exported);
            }

            if (is_exported) {
                error(peek(), "Expect function or variable declaration after @export.");
            }

            if (match({TokenType::RETURN})) {
                return parse_return_statement();
            }

            return parse_expression_statement();
        }

        std::unique_ptr<Stmt> Parser::parse_import_statement() {
            Token import_keyword = consume(TokenType::IDENTIFIER, "Expect 'import' after '#'.");
            if (import_keyword.lexeme != "import") {
                error(import_keyword, "Expected 'import' keyword.");
            }
            Token path = consume(TokenType::STRING_LITERAL, "Expect file path after #import.");
            return std::make_unique<ImportStmt>(path);
        }

        std::unique_ptr<Stmt> Parser::parse_variable_declaration(bool is_exported) {
            Token keyword = previous();
            bool is_mutable = (keyword.type == TokenType::VAR);
            Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");

            std::unique_ptr<Expr> type_expr = nullptr;
            if (match({TokenType::COLON})) {
                type_expr = parse_expression();
            }

            std::unique_ptr<Expr> initializer = nullptr;
            if (match({TokenType::EQUAL})) {
                initializer = parse_expression();
            }
            
            bool final_exported = is_exported || m_export_all;
            return std::make_unique<VarStmt>(name, std::move(type_expr), std::move(initializer), is_mutable, final_exported);
        }

        std::unique_ptr<Stmt> Parser::parse_function_statement(bool is_exported) {
            Token name = consume(TokenType::IDENTIFIER, "Expect function name.");
            consume(TokenType::LEFT_PAREN, "Expect '(' after function name.");
            std::vector<Token> parameters;
            if (!check(TokenType::RIGHT_PAREN)) {
                do {
                    parameters.push_back(consume(TokenType::IDENTIFIER, "Expect parameter name."));
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");
            
            bool final_exported = is_exported || m_export_all;

            // If there is a body, it's a definition. Otherwise, it's a declaration.
            if (match({TokenType::LEFT_BRACE})) {
                // Handle nested function export rules
                bool parent_export_all = m_export_all;
                m_export_all = false; // Disable @exportall for nested items

                std::vector<std::unique_ptr<Stmt>> body;
                while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
                    body.push_back(parse_statement());
                }
                consume(TokenType::RIGHT_BRACE, "Expect '}' after function body.");

                m_export_all = parent_export_all; // Restore parent's @exportall context

                return std::make_unique<FunctionStmt>(name, parameters, std::move(body), final_exported);
            } else {
                // No body, so it's a declaration
                return std::make_unique<FunctionDeclStmt>(name, parameters, final_exported);
            }
        }

        std::unique_ptr<Stmt> Parser::parse_return_statement() {
            Token keyword = previous();
            std::unique_ptr<Expr> value = parse_expression();
            return std::make_unique<ReturnStmt>(keyword, std::move(value));
        }

        std::unique_ptr<Stmt> Parser::parse_expression_statement() {
            std::unique_ptr<Expr> expr = parse_expression();
            return std::make_unique<ExprStmt>(std::move(expr));
        }

        // --- Expression Parsing ---

        std::unique_ptr<Expr> Parser::parse_expression() {
            return parse_assignment();
        }

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
            if (match({TokenType::MINUS})) {
                // Simplified unary expression
            }
            return parse_call();
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
            if (match({TokenType::STRING_LITERAL, TokenType::NUMBER_LITERAL})) {
                return std::make_unique<LiteralExpr>(previous());
            }
            if (match({TokenType::IDENTIFIER})) {
                return std::make_unique<VariableExpr>(previous());
            }
            if (match({TokenType::LEFT_PAREN})) {
                Token paren_token = previous();
                std::unique_ptr<Expr> expr = parse_expression();
                consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
                return std::make_unique<GroupingExpr>(paren_token, std::move(expr));
            }
            error(peek(), "Expect expression.");
            return nullptr; // Should be unreachable
        }

        // --- Helper Methods ---

        bool Parser::is_at_end() { return peek().type == TokenType::END_OF_FILE; }
        Token& Parser::peek() { return m_tokens[m_current]; }
        Token& Parser::previous() { return m_tokens[m_current - 1]; }
        Token& Parser::advance() { if (!is_at_end()) m_current++; return previous(); }
        bool Parser::check(TokenType type) { if (is_at_end()) return false; return peek().type == type; }

        bool Parser::match(const std::vector<TokenType>& types) {
            for (TokenType type : types) {
                if (check(type)) {
                    advance();
                    return true;
                }
            }
            return false;
        }

        Token& Parser::consume(TokenType type, const std::string& message) {
            if (check(type)) return advance();
            error(peek(), message);
            throw std::runtime_error("Unreachable"); // To satisfy compiler
        }

        void Parser::error(const Token& token, const std::string& message) {
            throw ParserError(message, token.line, token.column);
        }

    }
}
