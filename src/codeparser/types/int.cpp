#include "../../../include/codeparser/types/int.h"

namespace Iodicium {
    namespace Types {

        Int::Int(int32_t value) : m_value(value) {}

        int32_t Int::getValue() const {
            return m_value;
        }

    }
}
