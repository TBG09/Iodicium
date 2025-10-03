#include "executable/ioe_reader.h"
#include <fstream>
#include <stdexcept>
#include <vector>

namespace Iodicium {
    namespace Executable {

        // File format constants from writer
        const uint32_t IOE_MAGIC_NUMBER = 0x45444F49; // 'IODE'
        const uint8_t IOE_VERSION = 0x01;

        IoeReader::IoeReader(Common::Logger& logger) : m_logger(logger) {
            m_logger.debug("IoeReader constructor called.");
        }

        Chunk IoeReader::readFromFile(const std::string& path) {
            m_logger.debug("IoeReader: Reading executable from: " + path);
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open()) {
                m_logger.error("IoeReader: Failed to open file for reading: " + path);
                throw IoeReaderError("Failed to open file: " + path);
            }

            Chunk chunk;

            // Read and validate the .iode header
            uint32_t magic_number;
            file.read(reinterpret_cast<char*>(&magic_number), sizeof(magic_number));
            if (magic_number != IOE_MAGIC_NUMBER) {
                throw IoeReaderError("Invalid .iode file: Incorrect magic number.");
            }

            uint8_t version;
            file.read(reinterpret_cast<char*>(&version), sizeof(version));
            if (version != IOE_VERSION) {
                throw IoeReaderError("Unsupported .iode file version: " + std::to_string(version));
            }

            // Read the import section (import table)
            uint32_t import_count;
            file.read(reinterpret_cast<char*>(&import_count), sizeof(import_count));
            m_logger.debug("IoeReader: Found " + std::to_string(import_count) + " external references.");
            chunk.external_references.reserve(import_count);
            for (uint32_t i = 0; i < import_count; ++i) {
                uint32_t path_length;
                file.read(reinterpret_cast<char*>(&path_length), sizeof(path_length));
                std::string import_path(path_length, '\0');
                file.read(&import_path[0], path_length);
                chunk.external_references.push_back(import_path);
            }

            // Read the data section (constant pool)
            uint32_t constant_count;
            file.read(reinterpret_cast<char*>(&constant_count), sizeof(constant_count));
            chunk.constants.reserve(constant_count);
            for (uint32_t i = 0; i < constant_count; ++i) {
                uint32_t constant_length;
                file.read(reinterpret_cast<char*>(&constant_length), sizeof(constant_length));
                std::string constant_data(constant_length, '\0'); // Pre-allocate string
                file.read(&constant_data[0], constant_length);
                chunk.constants.push_back(constant_data);
            }

            // Read the code section (bytecode)
            uint32_t code_size;
            file.read(reinterpret_cast<char*>(&code_size), sizeof(code_size));
            chunk.code.resize(code_size);
            if (code_size > 0) {
                file.read(reinterpret_cast<char*>(chunk.code.data()), code_size);
            }

            file.close();
            m_logger.debug("IoeReader: File closed: " + path);
            return chunk;
        }

    }
}
