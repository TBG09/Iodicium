#include "cppToml/types/TomlDateTime.hpp"

namespace cppToml {

std::unique_ptr<DateTimeValue> parseDateTime(const std::string& literal_content) {
    return std::make_unique<DateTimeValue>(literal_content);
}

}
