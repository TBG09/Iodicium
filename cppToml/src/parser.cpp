#include "cppToml/parser.hpp"
#include "cppToml/types/TomlString.hpp"
#include "cppToml/types/TomlInteger.hpp"
#include "cppToml/types/TomlFloat.hpp"
#include "cppToml/types/TomlBoolean.hpp"
#include "cppToml/types/TomlDateTime.hpp"
#include "cppToml/types/TomlArray.hpp"
#include <stdexcept>

namespace cppToml {

Parser::Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens)) {}

std::unique_ptr<RootNode> Parser::parse() {
    auto root = std::make_unique<RootNode>();
    TableNode* current_table = nullptr;

    while (!isAtEnd()) {
        if (peek().type == TokenType::LEFT_BRACKET) {
            if (peekNext().type == TokenType::LEFT_BRACKET) {
                // Array of tables logic would go here
            } else {
                auto table = parseTable();
                std::string name = table->name;
                root->tables[name] = std::move(table);
                current_table = root->tables[name].get();
            }
        } else if (peek().type == TokenType::IDENTIFIER) {
            auto kv = parseKeyValue();
            if (current_table) {
                current_table->values[kv->key] = std::move(kv);
            } else {
                root->top_level_values[kv->key] = std::move(kv);
            }
        } else if (peek().type == TokenType::NEWLINE) {
            advance(); // Skip blank lines
        } else if (peek().type == TokenType::END_OF_FILE) {
            break; // All done
        } else {
            throw std::runtime_error("Unexpected token in TOML file: " + peek().lexeme);
        }
    }

    return root;
}

std::unique_ptr<KeyValueNode> Parser::parseKeyValue() {
    Token key_token = advance(); // Consume IDENTIFIER
    consume(TokenType::EQUALS, "Expected '=' after key.");
    auto value = parseValue();
    return std::make_unique<KeyValueNode>(key_token.lexeme, std::move(value));
}

std::unique_ptr<Value> Parser::parseValue() {
    Token value_token = peek();
    switch (value_token.type) {
        case TokenType::STRING: {
            advance(); // Consume the token
            std::string content = value_token.lexeme.substr(1, value_token.lexeme.length() - 2);
            return parseString(content);
        }
        case TokenType::INTEGER: {
            advance();
            return parseInteger(value_token.lexeme);
        }
        case TokenType::FLOAT: {
            advance();
            return parseFloat(value_token.lexeme);
        }
        case TokenType::BOOLEAN: {
            advance();
            return parseBoolean(value_token.lexeme);
        }
        case TokenType::DATE_TIME: {
            advance();
            return parseDateTime(value_token.lexeme);
        }
        case TokenType::LEFT_BRACKET: { // This indicates the start of an array
            return parseArray(*this);
        }
        default:
            throw std::runtime_error("Unexpected value type: " + value_token.lexeme);
    }
}

std::unique_ptr<TableNode> Parser::parseTable() {
    consume(TokenType::LEFT_BRACKET, "Expected '[' to start table.");
    Token name = advance(); // Consume IDENTIFIER
    consume(TokenType::RIGHT_BRACKET, "Expected ']' to end table.");

    auto table = std::make_unique<TableNode>();
    table->name = name.lexeme;
    return table;
}

// --- Helper Methods ---

bool Parser::isAtEnd() {
    return m_current >= m_tokens.size() || peek().type == TokenType::END_OF_FILE;
}

Token& Parser::advance() {
    if (!isAtEnd()) m_current++;
    return m_tokens[m_current - 1];
}

Token& Parser::peek() {
    return m_tokens[m_current];
}

Token& Parser::peekNext() {
    if (m_current + 1 >= m_tokens.size()) return m_tokens.back(); // Should be EOF
    return m_tokens[m_current + 1];
}

void Parser::consume(TokenType type, const std::string& message) {
    if (peek().type == type) {
        advance();
        return;
    }
    throw std::runtime_error(message + " Got '" + peek().lexeme + "' instead.");
}

}
