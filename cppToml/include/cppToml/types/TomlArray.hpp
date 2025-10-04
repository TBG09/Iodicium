#pragma once

#include "../value.hpp"
#include <string>
#include <memory>

namespace cppToml {

// Forward declare the Parser class to avoid circular dependencies
class Parser;

// Parses the content of a TOML array.
// It requires a reference to the main parser to recursively parse the values within the array.
std::unique_ptr<ArrayValue> parseArray(Parser& parser);

}
