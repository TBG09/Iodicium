#pragma once

#include <string>

namespace cppParse {

class Parser; // Forward declaration

class HelpFormatter {
public:
    explicit HelpFormatter(const Parser& parser);

    std::string format() const;

private:
    const Parser& m_parser;
};

}
