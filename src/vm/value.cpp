#include "vm/value.h"
#include <variant>
#include <sstream>

namespace Iodicium {
    namespace VM {

        std::ostream& operator<<(std::ostream& os, const Value& value) {
            std::visit([&](auto&& arg) {
                os << arg;
            }, value.data);
            return os;
        }

        std::string Value::toString() const {
            std::stringstream ss;
            std::visit([&](auto&& arg) {
                ss << arg;
            }, data);
            return ss.str();
        }

    } // namespace VM
} // namespace Iodicium
