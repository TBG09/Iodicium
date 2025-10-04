#ifndef IODICIUM_CODEPARSER_PARSER_H
#define IODICIUM_CODEPARSER_PARSER_H

#include <vector>
#include <memory>
#include <stdexcept>
#include "codeparser/ast.h"
#include "common/error.h" // Include the base error class
#include "common/logger.h" // Include logger header
#include "common/exceptions.h" // New: For specific parser exceptions

namespace Iodicium {
    namespace Codeparser {

        // Custom exception for parser errors, now inheriting from Common::CompilerException
        class ParserError : public Common::CompilerException {
        public:
            ParserError(const std::string& message, int line, int column)
                : Common::CompilerException(message, line, column) {}
        };

        class Parser {
        public:
            explicit Parser(std::vector<Token> tokens, Common::Logger& logger); // Added logger parameter

            std::vector<std::unique_ptr<Stmt>> parse();

        private:
            std::vector<Token> m_tokens;
            int m_current = 0;
            bool m_export_all = false;
            Common::Logger& m_logger; // Added logger member

            // Core parsing methods
            std::unique_ptr<Stmt> parse_statement();
            std::unique_ptr<Stmt> parse_import_statement();
            std::unique_ptr<Stmt> parse_variable_declaration(bool is_exported);
            std::unique_ptr<Stmt> parse_function_statement(bool is_exported);
            std::unique_ptr<Stmt> parse_return_statement();
            std::unique_ptr<Stmt> parse_expression_statement();

            // Expression parsing
            std::unique_ptr<Expr> parse_expression();
            std::unique_ptr<Expr> parse_assignment();
            std::unique_ptr<Expr> parse_term();
            std::unique_ptr<Expr> parse_factor();
            std::unique_ptr<Expr> parse_unary();
            std::unique_ptr<Expr> parse_call();
            std::unique_ptr<Expr> parse_primary();

            // Helper methods
            bool is_at_end();
            Token& peek();
            Token& previous();
            Token& advance();
            bool check(TokenType type);
            bool match(const std::vector<TokenType>& types);
            Token& consume(TokenType type, const std::string& message);
            void error(const Token& token, const std::string& message);
        };

    }
}

#endif //IODICIUM_CODEPARSER_PARSER_H
