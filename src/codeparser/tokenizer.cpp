#include "codeparser/tokenizer.h"

namespace Iodicium {
    namespace Codeparser {

        std::string to_string(TokenType type) {
            switch (type) {
                case TokenType::DEF: return "DEF";
                case TokenType::RETURN: return "RETURN";
                case TokenType::VAL: return "VAL";
                case TokenType::VAR: return "VAR";
                case TokenType::IDENTIFIER: return "IDENTIFIER";
                case TokenType::STRING_LITERAL: return "STRING_LITERAL";
                case TokenType::NUMBER_LITERAL: return "NUMBER_LITERAL";
                case TokenType::MINUS: return "MINUS";
                case TokenType::PLUS: return "PLUS";
                case TokenType::SLASH: return "SLASH";
                case TokenType::STAR: return "STAR";
                case TokenType::EQUAL: return "EQUAL";
                case TokenType::LEFT_PAREN: return "LEFT_PAREN";
                case TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
                case TokenType::COLON: return "COLON";
                case TokenType::ARROW: return "ARROW";
                case TokenType::NEWLINE: return "NEWLINE";
                case TokenType::INDENT: return "INDENT";
                case TokenType::DEDENT: return "DEDENT";
                case TokenType::END_OF_FILE: return "END_OF_FILE";
                case TokenType::TOKEN_ERROR: return "TOKEN_ERROR";
            }
            return "UNKNOWN";
        }

        std::ostream& operator<<(std::ostream& os, const Token& token) {
            os << "Token( " << to_string(token.type) << ", '" << token.lexeme << "', line " << token.line << " )";
            return os;
        }

    }
}
