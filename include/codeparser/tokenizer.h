#ifndef IODICIUM_CODEPARSER_TOKENIZER_H
#define IODICIUM_CODEPARSER_TOKENIZER_H

#include <string>
#include <iostream>

namespace Iodicium {
    namespace Codeparser {

        enum class TokenType {
            // Keywords
            DEF, RETURN, VAL, VAR,

            // Identifiers and Literals
            IDENTIFIER, STRING_LITERAL, NUMBER_LITERAL,

            // Operators
            MINUS, PLUS, SLASH, STAR, EQUAL,

            // Punctuation
            LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
            COLON, COMMA, ARROW,
            HASH, // Added for #import
            AT,   // Added for @export

            // Special Tokens
            NEWLINE, INDENT, DEDENT, END_OF_FILE,
            TOKEN_ERROR // Renamed from ERROR to avoid collision with windows.h
        };

        struct Token {
            TokenType type;
            std::string lexeme;
            int line;
            int column; // Added column member
        };

        // Operator for printing tokens, useful for debugging
        std::ostream& operator<<(std::ostream& os, const Token& token);

    }
}

#endif //IODICIUM_CODEPARSER_TOKENIZER_H
