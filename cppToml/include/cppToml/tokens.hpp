#pragma once

#include <string>

namespace cppToml {

enum class TokenType {
    // Punctuation
    EQUALS,          // =
    LEFT_BRACKET,    // [
    RIGHT_BRACKET,   // ]
    LEFT_BRACE,      // {
    RIGHT_BRACE,     // }
    COMMA,           // ,
    DOT,             // .

    // Literals
    STRING,
    INTEGER,
    FLOAT,
    BOOLEAN,
    DATE_TIME,

    // Keywords (although TOML has few, true/false are handled as BOOLEAN)

    // Misc
    IDENTIFIER,      // For keys
    NEWLINE,
    END_OF_FILE,
    UNRECOGNIZED
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line = 0;
    int column = 0;
};

}
