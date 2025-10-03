#ifndef IODICIUM_COMPILER_LINKER_H
#define IODICIUM_COMPILER_LINKER_H

#include <string>
#include <vector>
#include <memory>
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

        private:
            Common::Logger& m_logger;
        };

    }
}

#endif //IODICIUM_COMPILER_LINKER_H
