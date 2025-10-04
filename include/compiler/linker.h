#ifndef IODICIUM_COMPILER_LINKER_H
#define IODICIUM_COMPILER_LINKER_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "common/logger.h"
#include "executable/ioe_reader.h" // For Chunk

namespace Iodicium {
    namespace Compiler {

        // The Linker is responsible for orchestrating the compilation of multiple
        // source files into a single, statically-linked executable chunk.
        class Linker {
        public:
            explicit Linker(Common::Logger& logger);

            // Takes a list of source file paths and produces a single, linked chunk.
            Executable::Chunk link(const std::vector<std::string>& source_paths);

            // Returns the map of function names to their instruction pointer addresses.
            const std::map<std::string, size_t>& getFunctionIPs() const { return m_function_ips; }

        private:
            Common::Logger& m_logger;
            std::map<std::string, size_t> m_function_ips;
        };

    }
}

#endif //IODICIUM_COMPILER_LINKER_H
