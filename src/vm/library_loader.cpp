#include "vm/library_loader.h"
#include "executable/iodl_reader.h"

namespace Iodicium {
    namespace VM {

        LibraryLoader::LibraryLoader(Common::Logger& logger) : m_logger(logger) {}

        void LibraryLoader::prepare(const Executable::Chunk& main_chunk, const std::string& base_path) {
            m_logger.info("LibraryLoader: Preparing external function references from pre-compiled libraries.");

            for (const auto& signature : main_chunk.external_references) {
                m_logger.debug("Processing external reference: " + signature);

                size_t separator_pos = signature.find(';');
                if (separator_pos == std::string::npos) {
                    throw std::runtime_error("Invalid external reference signature: " + signature);
                }

                std::string library_path_rel = signature.substr(0, separator_pos);
                std::string function_name = signature.substr(separator_pos + 1);
                std::string library_path_abs = base_path + "/" + library_path_rel;

                // Load the library if it hasn't been already
                if (m_loaded_libraries.find(library_path_abs) == m_loaded_libraries.end()) {
                    m_logger.info("Loading library: " + library_path_abs);
                    Executable::IodlReader reader(m_logger);
                    auto lib_chunk = std::make_unique<Executable::LibraryChunk>(reader.readFromFile(library_path_abs));
                    m_loaded_libraries[library_path_abs] = std::move(lib_chunk);
                }

                // Find the function in the now-loaded library
                Executable::LibraryChunk* library = m_loaded_libraries.at(library_path_abs).get();
                auto it = library->exports.find(function_name);
                if (it == library->exports.end()) {
                    throw std::runtime_error("Function '" + function_name + "' not found in library '" + library_path_abs + "'.");
                }

                // Store the direct function reference
                LoadedFunction func_ref;
                func_ref.library = library;
                func_ref.ip = it->second;
                m_function_map.push_back(func_ref);
            }
            m_logger.info("LibraryLoader: All external references prepared.");
        }

        const LoadedFunction& LibraryLoader::getFunction(uint8_t ordinal) {
            if (ordinal >= m_function_map.size()) {
                throw std::runtime_error("Invalid external function ordinal: " + std::to_string(ordinal));
            }
            return m_function_map[ordinal];
        }

    }
}
