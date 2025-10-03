#include "codeparser/lexer.h"
#include <map>
#include <cctype> // For std::isalpha, std::isdigit, std::isalnum

namespace Iodicium {
    namespace Codeparser {

        static const std::map<std::string, TokenType> keywords = {
            {"def",    TokenType::DEF},
            {"return", TokenType::RETURN},
            {"val",    TokenType::VAL},
            {"var",    TokenType::VAR}
        };

        Lexer::Lexer(std::string source, Common::Logger& logger) : m_source(std::move(source)), m_line_start_index(0), m_logger(logger) {}

        std::vector<Token> Lexer::tokenize() {
            while (!is_at_end()) {
                m_start = m_current;
                scan_token();
            }
            m_tokens.push_back({TokenType::END_OF_FILE, "", m_line, m_current - m_line_start_index + 1});
            return m_tokens;
        }

        bool Lexer::is_at_end() {
            return m_current >= m_source.length();
        }

        char Lexer::advance() {
            return m_source[m_current++];
        }

        char Lexer::peek() {
            if (is_at_end()) return '\0';
            return m_source[m_current];
        }

        char Lexer::peek_next() {
            if (m_current + 1 >= m_source.length()) return '\0';
            return m_source[m_current + 1];
        }

        char Lexer::previous() {
            return m_source[m_current - 1];
        }

        void Lexer::add_token(TokenType type) {
            std::string text = m_source.substr(m_start, m_current - m_start);
            m_tokens.push_back({type, text, m_line, m_start - m_line_start_index + 1});
        }

        void Lexer::add_token(TokenType type, const std::string& literal) {
            m_tokens.push_back({type, literal, m_line, m_start - m_line_start_index + 1});
        }

        void Lexer::scan_token() {
            char c = advance();
            m_logger.debug("Lexer: Starting scan_token for char '" + std::string(1, c) + "' (ASCII: " + std::to_string(static_cast<int>(c)) + ") at line " + std::to_string(m_line) + ", column " + std::to_string(m_current - m_line_start_index));

            switch (c) {
                case '(': add_token(TokenType::LEFT_PAREN); break;
                case ')': add_token(TokenType::RIGHT_PAREN); break;
                case '{': add_token(TokenType::LEFT_BRACE); break;
                case '}': add_token(TokenType::RIGHT_BRACE); break;
                case ',': add_token(TokenType::COMMA); break;
                case ':': add_token(TokenType::COLON); break;
                case '+': add_token(TokenType::PLUS); break;
                case '*': add_token(TokenType::STAR); break;
                case '=': add_token(TokenType::EQUAL); break;
                case '@': add_token(TokenType::AT); break;
                case '#': add_token(TokenType::HASH); break;
                case '-':
                    handle_two_char_token('>', TokenType::ARROW, TokenType::MINUS);
                    break;
                case '/':
                    if (peek() == '/') {
                        while (peek() != '\n' && !is_at_end()) advance();
                    } else {
                        add_token(TokenType::SLASH);
                    }
                    break;
                case ' ': case '\r': case '\t': break;
                case '\n':
                    m_line++;
                    m_line_start_index = m_current;
                    break;
                case '"' : handle_string(); break;
                default:
                    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                        handle_identifier();
                    } else if (std::isdigit(static_cast<unsigned char>(c))) {
                        handle_number();
                    } else {
                        throw LexerError("Unexpected character: '" + std::string(1, c) + "'.", m_line, m_start - m_line_start_index + 1);
                    }
            }
        }

        void Lexer::handle_two_char_token(char expected, TokenType two_char_type, TokenType one_char_type) {
            if (peek() == expected) {
                advance();
                add_token(two_char_type);
            } else {
                add_token(one_char_type);
            }
        }

        void Lexer::handle_string() {
            std::string value;
            while (peek() != '"' && !is_at_end()) {
                char c = advance();
                if (c == '\\') { // Escape sequence
                    switch (advance()) {
                        case 'n': value += '\n'; break;
                        case 't': value += '\t'; break;
                        case '\\': value += '\\'; break;
                        case '"': value += '"'; break;
                        default:
                            // Or throw an error for unsupported escape sequences
                            value += '\\'; // Just add the backslash
                            value += previous(); // And the character after it
                            break;
                    }
                } else {
                    value += c;
                }
            }
            if (is_at_end()) {
                throw LexerError("Unterminated string.", m_line, m_start - m_line_start_index + 1);
            }
            advance(); // Consume the closing '"'
            add_token(TokenType::STRING_LITERAL, value);
        }

        void Lexer::handle_identifier() {
            while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') advance();
            std::string text = m_source.substr(m_start, m_current - m_start);
            auto it = keywords.find(text);
            if (it != keywords.end()) {
                add_token(it->second);
            } else {
                add_token(TokenType::IDENTIFIER);
            }
        }

        void Lexer::handle_number() {
            while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
            if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek_next()))) {
                advance();
                while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
            }
            add_token(TokenType::NUMBER_LITERAL);
        }

    }
}
