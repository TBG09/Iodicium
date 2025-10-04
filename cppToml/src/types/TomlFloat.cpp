#include "cppToml/types/TomlFloat.hpp"
#include <string>
#include <algorithm> // For std::remove

namespace cppToml {

std::unique_ptr<FloatValue> parseFloat(const std::string& literal_content) {
    std::string clean_str = literal_content;
    clean_str.erase(std::remove(clean_str.begin(), clean_str.end(), '_'), clean_str.end());

    double val = std::stod(clean_str);
    return std::make_unique<FloatValue>(val);
}

}
