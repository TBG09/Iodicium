#include "cppToml/types/TomlInteger.hpp"
#include <string>
#include <algorithm> // For std::remove

namespace cppToml {

std::unique_ptr<IntegerValue> parseInteger(const std::string& literal_content) {
    std::string clean_str = literal_content;
    // Remove all underscores from the string for correct parsing
    clean_str.erase(std::remove(clean_str.begin(), clean_str.end(), '_'), clean_str.end());

    int64_t val = std::stoll(clean_str);
    return std::make_unique<IntegerValue>(val);
}

}
