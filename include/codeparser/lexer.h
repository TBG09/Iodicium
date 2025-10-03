#ifndef IODICIUM_CODEPARSER_LEXER_H
#define IODICIUM_CODEPARSER_LEXER_H

#include <vector>
#include <string>
#include "codeparser/tokenizer.h"
#include "common/logger.h"
#include "common/error.h"

namespace Iodicium {
    namespace Codeparser {

        class LexerError : public Common::IodiciumError {
        public:
            LexerError(const std::string& message, int line, int column)
                : Common::IodiciumError(message, line, column) {}
        };

        class Lexer {
        public:
            explicit Lexer(std::string source, Common::Logger& logger);
            std::vector<Token> tokenize();

        private:
            std::string m_source;
            std::vector<Token> m_tokens;
            int m_start = 0;
            int m_current = 0;
            int m_line = 1;
            int m_line_start_index = 0;
            Common::Logger& m_logger;

            void scan_token();
            bool is_at_end();
            char advance();
            char peek();
            char peek_next();
            char previous();
            void add_token(TokenType type);
            void add_token(TokenType type, const std::string& literal);
            void handle_two_char_token(char expected, TokenType two_char_type, TokenType one_char_type);
            void handle_string();
            void handle_identifier();
            void handle_number();
        };

    }
}

#endif //IODICIUM_CODEPARSER_LEXER_H
