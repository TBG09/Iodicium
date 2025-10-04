#include "cppToml/types/TomlArray.hpp"
#include "cppToml/parser.hpp" // Include the main parser for recursive calls

namespace cppToml {

std::unique_ptr<ArrayValue> parseArray(Parser& parser) {
    auto array = std::make_unique<ArrayValue>();

    parser.consume(TokenType::LEFT_BRACKET, "Expected '[' to start an array.");

    if (parser.peek().type == TokenType::RIGHT_BRACKET) {
        parser.consume(TokenType::RIGHT_BRACKET, "Expected ']' to end an empty array.");
        return array;
    }

    while (parser.peek().type != TokenType::RIGHT_BRACKET) {
        array->values.push_back(parser.parseValue());

        if (parser.peek().type == TokenType::COMMA) {
            parser.consume(TokenType::COMMA, "Expected comma after array element.");
        } else if (parser.peek().type != TokenType::RIGHT_BRACKET) {
            throw std::runtime_error("Expected comma or ']' after array element.");
        }
    }

    parser.consume(TokenType::RIGHT_BRACKET, "Expected ']' to end the array.");

    return array;
}

}
