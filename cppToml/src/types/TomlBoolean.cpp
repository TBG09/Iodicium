#include "cppToml/types/TomlBoolean.hpp"
#include <stdexcept>

namespace cppToml {

std::unique_ptr<BooleanValue> parseBoolean(const std::string& literal_content) {
    if (literal_content == "true") {
        return std::make_unique<BooleanValue>(true);
    } else if (literal_content == "false") {
        return std::make_unique<BooleanValue>(false);
    } else {
        throw std::runtime_error("Invalid boolean value: " + literal_content);
    }
}

}
