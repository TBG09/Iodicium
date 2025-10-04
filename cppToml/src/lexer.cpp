#include "cppToml/lexer.hpp"
#include <stdexcept> 
#include <cctype>

namespace cppToml {

Lexer::Lexer(const std::string& source) : m_source(source) {}

std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        m_start = m_current;
        scanToken();
    }
    m_tokens.push_back({TokenType::END_OF_FILE, "", m_line, m_current - m_line_start + 1});
    return m_tokens;
}

bool Lexer::isAtEnd() {
    return m_current >= m_source.length();
}

char Lexer::advance() {
    m_current++;
    return m_source[m_current - 1];
}

char Lexer::peek() {
    if (isAtEnd()) return '\0';
    return m_source[m_current];
}

char Lexer::peekNext() {
    if (m_current + 1 >= m_source.length()) return '\0';
    return m_source[m_current + 1];
}

void Lexer::addToken(TokenType type) {
    std::string text = m_source.substr(m_start, m_current - m_start);
    Token token = {type, text, m_line, m_start - m_line_start + 1};
    m_tokens.push_back(token);
}

// This overload is for tokens where the lexeme is different from the consumed text (e.g. parsed strings)
void Lexer::addToken(TokenType type, const std::string& literal) {
    std::string text = m_source.substr(m_start, m_current - m_start);
    Token token = {type, text, m_line, m_start - m_line_start + 1};
    m_tokens.push_back(token);
}

void Lexer::scanToken() {
    char c = advance();
    switch (c) {
        case '=': addToken(TokenType::EQUALS); break;
        case '[': addToken(TokenType::LEFT_BRACKET); break;
        case ']': addToken(TokenType::RIGHT_BRACKET); break;
        case '{': addToken(TokenType::LEFT_BRACE); break;
        case '}': addToken(TokenType::RIGHT_BRACE); break;
        case ',': addToken(TokenType::COMMA); break;
        case '.': addToken(TokenType::DOT); break;
        case ' ': case '\r': case '\t': break; // Ignore whitespace
        case '\n': m_line++; m_line_start = m_current; break;
        case '#': while (peek() != '\n' && !isAtEnd()) advance(); break; // Comments

        case '\'': // Literal strings
            if (peek() == '\'' && peekNext() == '\'') {
                advance(); advance(); // Consume the other two quotes
                handleMultilineLiteralString();
            } else {
                handleLiteralString();
            }
            break;

        case '"': // Basic strings
            if (peek() == '"' && peekNext() == '"' ) {
                advance(); advance();
                handleMultilineBasicString();
            } else {
                handleBasicString();
            }
            break;

        default:
            if (std::isalpha(c) || c == '_') {
                handleIdentifierOrBoolean();
            } else if (std::isdigit(c) || c == '-' || c == '+') {
                handleNumberOrDate();
            } else {
                addToken(TokenType::UNRECOGNIZED);
            }
            break;
    }
}

// --- Specific Token Handlers ---

void Lexer::handleBasicString() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') throw std::runtime_error("Line breaks are not allowed in basic strings.");
        if (peek() == '\\') advance(); // Skip escaped char
        advance();
    }
    if (isAtEnd()) throw std::runtime_error("Unterminated string.");
    advance(); // Closing quote
    addToken(TokenType::STRING);
}

void Lexer::handleMultilineBasicString() {
    while (!(peek() == '"' && peekNext() == '"' && m_source[m_current+2] == '"') && !isAtEnd()) {
        if (peek() == '\n') m_line++;
        advance();
    }
    if (isAtEnd()) throw std::runtime_error("Unterminated multiline string.");
    advance(); advance(); advance(); // Closing quotes
    addToken(TokenType::STRING);
}

void Lexer::handleLiteralString() {
    while (peek() != '\'' && !isAtEnd()) {
        if (peek() == '\n') throw std::runtime_error("Line breaks are not allowed in literal strings.");
        advance();
    }
    if (isAtEnd()) throw std::runtime_error("Unterminated literal string.");
    advance(); // Closing quote
    addToken(TokenType::STRING);
}

void Lexer::handleMultilineLiteralString() {
    while (!(peek() == '\'' && peekNext() == '\'' && m_source[m_current+2] == '\'') && !isAtEnd()) {
        if (peek() == '\n') m_line++;
        advance();
    }
    if (isAtEnd()) throw std::runtime_error("Unterminated multiline literal string.");
    advance(); advance(); advance(); // Closing quotes
    addToken(TokenType::STRING);
}

void Lexer::handleIdentifierOrBoolean() {
    while (std::isalnum(peek()) || peek() == '_' || peek() == '-') advance();
    std::string text = m_source.substr(m_start, m_current - m_start);
    if (text == "true" || text == "false") {
        addToken(TokenType::BOOLEAN);
    } else {
        addToken(TokenType::IDENTIFIER);
    }
}

void Lexer::handleNumberOrDate() {
    bool is_date = false;
    int hyphens = 0;
    int colons = 0;

    while (!isAtEnd() && (std::isdigit(peek()) || peek() == '-' || peek() == ':' || peek() == '.' || peek() == 'T' || peek() == 'Z')) {
        if (peek() == '-') hyphens++;
        if (peek() == ':') colons++;
        if (peek() == 'T' || peek() == 'Z') is_date = true;
        advance();
    }

    if (is_date || (hyphens >= 2) || (colons >= 2)) {
        addToken(TokenType::DATE_TIME);
        return;
    }

    m_current = m_start + 1;
    while (std::isdigit(peek())) advance();
    if (peek() == '.' && std::isdigit(peekNext())) {
        advance();
        while (std::isdigit(peek())) advance();
        addToken(TokenType::FLOAT);
    } else {
        addToken(TokenType::INTEGER);
    }
}

}
