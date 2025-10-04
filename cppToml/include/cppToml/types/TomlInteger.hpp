#pragma once

#include "../value.hpp"
#include <string>
#include <memory>

namespace cppToml {

// Parses the content of a TOML integer literal, handling underscores.
std::unique_ptr<IntegerValue> parseInteger(const std::string& literal_content);

}
