#ifndef IODICIUM_COMMON_LOGGER_H
#define IODICIUM_COMMON_LOGGER_H

#include <string>
#include <sstream>
#include "iod_common_export.h"

namespace Iodicium {
    namespace Common {

        enum class LogLevel {
            Info,
            Warn,
            Error,
            Debug
        };

        class IOD_COMMON_API Logger {
        public:
            Logger();

            void setLevel(LogLevel level);
            bool isEnabled(LogLevel level) const;
            LogLevel getLevel() const; // Added getLevel() declaration

            void log(LogLevel level, const std::string& message);

            // Convenience methods
            void info(const std::string& message);
            void warn(const std::string& message);
            void error(const std::string& message);
            void debug(const std::string& message);

        private:
            LogLevel m_level = LogLevel::Info;
            bool m_colors_enabled = false;

            void detect_color_support();
        };

    }
}

#endif //IODICIUM_COMMON_LOGGER_H
