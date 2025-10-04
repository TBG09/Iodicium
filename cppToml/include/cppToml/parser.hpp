#pragma once

#include "cppToml/tokens.hpp"
#include "cppToml/ast.hpp"
#include <string>
#include <vector>
#include <memory>

namespace cppToml {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // The main entry point for parsing. Returns the root of the AST.
    std::unique_ptr<RootNode> parse();

    // Public method to parse a value, used for recursion (e.g., in arrays)
    std::unique_ptr<Value> parseValue();

    // Public helper to consume a token, needed by modular parsers
    void consume(TokenType type, const std::string& message);
    Token& peek();

private:
    std::vector<Token> m_tokens;
    int m_current = 0;

    // Helper methods for the parsing process
    bool isAtEnd();
    Token& advance();
    Token& peekNext(); // Added missing declaration

    // Grammar-specific parsing methods
    std::unique_ptr<KeyValueNode> parseKeyValue();
    std::unique_ptr<TableNode> parseTable();
};

}
