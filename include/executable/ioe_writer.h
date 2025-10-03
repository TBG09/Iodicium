#ifndef IODICIUM_EXECUTABLE_IOE_WRITER_H
#define IODICIUM_EXECUTABLE_IOE_WRITER_H

#include <string>
#include <vector>
#include <cstdint>
#include "iod_executable_export.h"
#include "common/logger.h" // Include logger header
#include "common/error.h"   // Include the base error class

namespace Iodicium {
    namespace Executable {

        // Custom exception for IoeWriter errors
        class IoeWriterError : public Common::IodiciumError {
        public:
            IoeWriterError(const std::string& message, int line = -1, int column = -1)
                : Common::IodiciumError(message, line, column) {}
        };

        // Assembles the different sections of an Iodicium program into the final .iode binary format.
        class IOD_EXECUTABLE_API IoeWriter {
        public:
            explicit IoeWriter(Common::Logger& logger); // Added logger parameter
            void setCode(std::vector<uint8_t> code);
            void addConstant(const std::string& constant);
            void setImports(const std::vector<std::string>& imports);

            // Writes the complete .iode file to the specified path.
            void writeToFile(const std::string& path);

        private:
            Common::Logger& m_logger; // Added logger member
            std::vector<uint8_t> m_code_section;
            std::vector<std::string> m_data_section; // Constant pool
            std::vector<std::string> m_import_section; // Import table
        };

    }
}

#endif //IODICIUM_EXECUTABLE_IOE_WRITER_H
