#pragma once

#include "../value.hpp"
#include <string>
#include <memory>

namespace cppToml {

// Parses the content of a TOML date-time literal.
std::unique_ptr<DateTimeValue> parseDateTime(const std::string& literal_content);

}
