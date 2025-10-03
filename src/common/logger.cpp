#include "common/logger.h"
#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <io.h>
#else
#include <unistd.h> // For isatty
#endif

namespace Iodicium {
    namespace Common {

        // ANSI Color Codes
        const char* RESET = "\033[0m";
        const char* BLUE = "\033[34m";
        const char* YELLOW = "\033[33m";
        const char* RED = "\033[31m";
        const char* GRAY = "\033[90m";

        Logger::Logger() {
            detect_color_support();
        }

        void Logger::detect_color_support() {
#ifdef _WIN32

            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut == INVALID_HANDLE_VALUE) {
                m_colors_enabled = false;
                return;
            }

            DWORD dwMode = 0;
            if (!GetConsoleMode(hOut, &dwMode)) {
                m_colors_enabled = false;
                return;
            }

            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            if (!SetConsoleMode(hOut, dwMode)) {
                m_colors_enabled = false;
                return;
            }
            m_colors_enabled = true;
#else
            // On Linux/macOS, we check if stdout is a TTY
            m_colors_enabled = (isatty(fileno(stdout)) != 0);
#endif
        }

        void Logger::setLevel(LogLevel level) {
            m_level = level;
        }

        bool Logger::isEnabled(LogLevel level) const {
            return static_cast<int>(level) <= static_cast<int>(m_level);
        }

        LogLevel Logger::getLevel() const {
            return m_level;
        }

        void Logger::log(LogLevel level, const std::string& message) {
            if (!isEnabled(level)) {
                return;
            }

            std::ostream& stream = (level == LogLevel::Error) ? std::cerr : std::cout;

            if (m_colors_enabled) {
                switch (level) {
                    case LogLevel::Info:  stream << BLUE << "[INFO]  " << RESET; break;
                    case LogLevel::Warn:  stream << YELLOW << "[WARN]  " << RESET; break;
                    case LogLevel::Error: stream << RED << "[ERROR] " << RESET; break;
                    case LogLevel::Debug: stream << GRAY << "[DEBUG] " << RESET; break;
                }
            } else {
                 switch (level) {
                    case LogLevel::Info:  stream << "[INFO]  "; break;
                    case LogLevel::Warn:  stream << "[WARN]  "; break;
                    case LogLevel::Error: stream << "[ERROR] "; break;
                    case LogLevel::Debug: stream << "[DEBUG] "; break;
                }
            }

            stream << message << std::endl;
        }

        void Logger::info(const std::string& message) { log(LogLevel::Info, message); }
        void Logger::warn(const std::string& message) { log(LogLevel::Warn, message); }
        void Logger::error(const std::string& message) { log(LogLevel::Error, message); }
        void Logger::debug(const std::string& message) { log(LogLevel::Debug, message); }

    }
}
