#include "cppToml/types/TomlString.hpp"
#include <stdexcept>

namespace cppToml {

std::unique_ptr<StringValue> parseString(const std::string& literal_content) {
    std::string result;
    for (size_t i = 0; i < literal_content.length(); ++i) {
        if (literal_content[i] == '\\') {
            if (i + 1 >= literal_content.length()) {
                throw std::runtime_error("Invalid escape sequence at end of string.");
            }
            i++; // Move to the character after the backslash
            switch (literal_content[i]) {
                case 'b':  result += '\b'; break;
                case 't':  result += '\t'; break;
                case 'n':  result += '\n'; break;
                case 'f':  result += '\f'; break;
                case 'r':  result += '\r'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;

                default:
                    result += '\\';
                    result += literal_content[i];
                    break;
            }
        } else {
            result += literal_content[i];
        }
    }
    return std::make_unique<StringValue>(result);
}

}
