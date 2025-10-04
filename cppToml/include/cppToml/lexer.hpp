#pragma once

#include "cppToml/tokens.hpp"
#include <string>
#include <vector>

namespace cppToml {

class Lexer {
public:
    explicit Lexer(const std::string& source);

    std::vector<Token> tokenize();

private:
    std::string m_source;
    std::vector<Token> m_tokens;
    int m_start = 0;
    int m_current = 0;
    int m_line = 1;
    int m_line_start = 0;

    // Core helpers
    void scanToken();
    bool isAtEnd();
    char advance();
    char peek();
    char peekNext();
    void addToken(TokenType type);
    void addToken(TokenType type, const std::string& literal);

    // Specific token handlers
    void handleBasicString();
    void handleMultilineBasicString();
    void handleLiteralString();
    void handleMultilineLiteralString();
    void handleIdentifierOrBoolean();
    void handleNumberOrDate();
};

}
