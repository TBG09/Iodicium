#ifndef IODICIUM_CODEPARSER_TYPES_INT_H
#define IODICIUM_CODEPARSER_TYPES_INT_H

#include <cstdint>

namespace Iodicium {
    namespace Types {

        class Int {
        public:
            Int() = default;
            explicit Int(int32_t value);

            int32_t getValue() const;

        private:
            int32_t m_value;
        };

    }
}

#endif //IODICIUM_CODEPARSER_TYPES_INT_H
