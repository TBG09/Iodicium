#pragma once

#include "../value.hpp"
#include <string>
#include <memory>

namespace cppToml {

// Parses the content of a TOML string literal, handling escape sequences.
std::unique_ptr<StringValue> parseString(const std::string& literal_content);

}
