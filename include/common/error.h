#ifndef IODICIUM_COMMON_ERROR_H
#define IODICIUM_COMMON_ERROR_H

#include <stdexcept>
#include <string>

namespace Iodicium {
    namespace Common {

        class IodiciumError : public std::runtime_error {
        public:
            IodiciumError(const std::string& message, int line = -1, int column = -1)
                : std::runtime_error(message),
                  m_line(line),
                  m_column(column) {}

            int getLine() const { return m_line; }
            int getColumn() const { return m_column; }

        private:
            int m_line;
            int m_column;
        };

    } // namespace Common
} // namespace Iodicium

#endif //IODICIUM_COMMON_ERROR_H
