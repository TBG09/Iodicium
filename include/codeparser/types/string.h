#ifndef IODICIUM_CODEPARSER_TYPES_STRING_H
#define IODICIUM_CODEPARSER_TYPES_STRING_H

#include <string>

namespace Iodicium {
    namespace Types {

        class String {
        public:
            String() = default;
            explicit String(std::string value);

            std::string getValue() const;

        private:
            std::string m_value;
        };

    }
}

#endif //IODICIUM_CODEPARSER_TYPES_STRING_H
