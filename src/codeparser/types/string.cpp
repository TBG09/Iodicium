#include "../../../include/codeparser/types/string.h"

namespace Iodicium {
    namespace Types {

        String::String(std::string value) : m_value(std::move(value)) {}

        std::string String::getValue() const {
            return m_value;
        }

    }
}
