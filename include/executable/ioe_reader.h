#ifndef IODICIUM_EXECUTABLE_IOE_READER_H
#define IODICIUM_EXECUTABLE_IOE_READER_H

#include <string>
#include <vector>
#include <cstdint>
#include "common/logger.h"
#include "common/error.h"

namespace Iodicium {
    namespace Executable {

        // Represents a compiled chunk of bytecode
        struct Chunk {
            std::vector<uint8_t> code;
            std::vector<std::string> constants;
            std::vector<std::string> external_references; // New: For imported function signatures
        };

        class IoeReaderError : public Common::IodiciumError {
        public:
            IoeReaderError(const std::string& message, int line = -1, int column = -1)
                : Common::IodiciumError(message, line, column) {}
        };

        class IoeReader {
        public:
            explicit IoeReader(Common::Logger& logger);
            Chunk readFromFile(const std::string& path);

        private:
            Common::Logger& m_logger;
        };

    }
}

#endif //IODICIUM_EXECUTABLE_IOE_READER_H
