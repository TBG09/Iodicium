#pragma once

#include "../value.hpp"
#include <string>
#include <memory>

namespace cppToml {

// Parses the content of a TOML float literal, handling underscores.
std::unique_ptr<FloatValue> parseFloat(const std::string& literal_content);

}
