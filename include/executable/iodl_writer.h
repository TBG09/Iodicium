#ifndef IODICIUM_EXECUTABLE_IODL_WRITER_H
#define IODICIUM_EXECUTABLE_IODL_WRITER_H

#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include "common/logger.h"
#include "common/error.h"

namespace Iodicium {
    namespace Executable {

        class IodlWriterError : public Common::IodiciumError {
        public:
            IodlWriterError(const std::string& message, int line = -1, int column = -1)
                : Common::IodiciumError(message, line, column) {}
        };

        // Assembles the different sections of an Iodicium library into the final .iodl binary format.
        class IodlWriter {
        public:
            explicit IodlWriter(Common::Logger& logger);
            void setCode(std::vector<uint8_t> code);
            void addConstant(const std::string& constant);
            void setExports(const std::map<std::string, size_t>& exports);

            // Writes the complete .iodl file to the specified path.
            void writeToFile(const std::string& path);

        private:
            Common::Logger& m_logger;
            std::vector<uint8_t> m_code_section;
            std::vector<std::string> m_data_section; // Constant pool
            std::map<std::string, size_t> m_export_section; // Export table (function name -> IP)
        };

    }
}

#endif //IODICIUM_EXECUTABLE_IODL_WRITER_H
