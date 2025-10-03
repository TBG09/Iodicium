#ifndef IODICIUM_VM_LIBRARY_LOADER_H
#define IODICIUM_VM_LIBRARY_LOADER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "common/logger.h"
#include "executable/iodl_reader.h" // Use the new IODL reader

namespace Iodicium {
    namespace VM {

        // Represents a direct reference to a loaded external function.
        struct LoadedFunction {
            Executable::LibraryChunk* library;
            size_t ip; // The instruction pointer to the function's code
        };

        class LibraryLoader {
        public:
            explicit LibraryLoader(Common::Logger& logger);

            // Parses the external references from the main chunk and loads the required libraries.
            void prepare(const Executable::Chunk& main_chunk, const std::string& base_path);

            // Retrieves a prepared function reference by its ordinal.
            const LoadedFunction& getFunction(uint8_t ordinal);

        private:
            Common::Logger& m_logger;

            // Maps an ordinal to a loaded function reference.
            std::vector<LoadedFunction> m_function_map;

            // Stores the loaded library chunks to avoid redundant loads.
            // The key is the library's path.
            std::map<std::string, std::unique_ptr<Executable::LibraryChunk>> m_loaded_libraries;
        };

    }
}

#endif //IODICIUM_VM_LIBRARY_LOADER_H
