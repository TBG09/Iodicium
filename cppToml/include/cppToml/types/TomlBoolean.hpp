#pragma once

#include "../value.hpp"
#include <string>
#include <memory>

namespace cppToml {

// Parses the content of a TOML boolean literal ("true" or "false").
std::unique_ptr<BooleanValue> parseBoolean(const std::string& literal_content);

}
