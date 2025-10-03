#ifndef IODICIUM_EXECUTABLE_IODL_READER_H
#define IODICIUM_EXECUTABLE_IODL_READER_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include "common/logger.h"
#include "common/error.h"
#include "executable/ioe_reader.h" // For Chunk

namespace Iodicium {
    namespace Executable {

        class IodlReaderError : public Common::IodiciumError {
        public:
            IodlReaderError(const std::string& message, int line = -1, int column = -1)
                : Common::IodiciumError(message, line, column) {}
        };

        // Represents the contents of a loaded .iodl library file.
        struct LibraryChunk {
            Chunk code_chunk; // Re-use the existing Chunk for code and constants
            std::map<std::string, size_t> exports; // The map of exported function names to their IPs
        };

        class IodlReader {
        public:
            explicit IodlReader(Common::Logger& logger);
            LibraryChunk readFromFile(const std::string& path);

        private:
            Common::Logger& m_logger;
        };

    }
}

#endif //IODICIUM_EXECUTABLE_IODL_READER_H
