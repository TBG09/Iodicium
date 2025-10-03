#include "executable/iodl_reader.h"
#include <fstream>

namespace Iodicium {
    namespace Executable {

        // File format constants from writer
        const uint32_t IODL_MAGIC_NUMBER = 0x4C444F49; // 'IODL'
        const uint8_t IODL_VERSION = 0x01;

        IodlReader::IodlReader(Common::Logger& logger) : m_logger(logger) {
            m_logger.debug("IodlReader constructor called.");
        }

        LibraryChunk IodlReader::readFromFile(const std::string& path) {
            m_logger.debug("IodlReader: Reading library from: " + path);
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open()) {
                throw IodlReaderError("Failed to open library file: " + path);
            }

            LibraryChunk lib_chunk;

            // Read and validate the .iodl header
            uint32_t magic_number;
            file.read(reinterpret_cast<char*>(&magic_number), sizeof(magic_number));
            if (magic_number != IODL_MAGIC_NUMBER) {
                throw IodlReaderError("Invalid .iodl file: Incorrect magic number.");
            }

            uint8_t version;
            file.read(reinterpret_cast<char*>(&version), sizeof(version));
            if (version != IODL_VERSION) {
                throw IodlReaderError("Unsupported .iodl file version: " + std::to_string(version));
            }

            // Read the export section (export table)
            uint32_t export_count;
            file.read(reinterpret_cast<char*>(&export_count), sizeof(export_count));
            for (uint32_t i = 0; i < export_count; ++i) {
                uint32_t name_length;
                file.read(reinterpret_cast<char*>(&name_length), sizeof(name_length));
                std::string name(name_length, '\0');
                file.read(&name[0], name_length);
                uint64_t ip;
                file.read(reinterpret_cast<char*>(&ip), sizeof(ip));
                lib_chunk.exports[name] = static_cast<size_t>(ip);
            }

            // Read the data section (constant pool)
            uint32_t constant_count;
            file.read(reinterpret_cast<char*>(&constant_count), sizeof(constant_count));
            lib_chunk.code_chunk.constants.reserve(constant_count);
            for (uint32_t i = 0; i < constant_count; ++i) {
                uint32_t constant_length;
                file.read(reinterpret_cast<char*>(&constant_length), sizeof(constant_length));
                std::string constant_data(constant_length, '\0');
                file.read(&constant_data[0], constant_length);
                lib_chunk.code_chunk.constants.push_back(constant_data);
            }

            // Read the code section (bytecode)
            uint32_t code_size;
            file.read(reinterpret_cast<char*>(&code_size), sizeof(code_size));
            lib_chunk.code_chunk.code.resize(code_size);
            if (code_size > 0) {
                file.read(reinterpret_cast<char*>(lib_chunk.code_chunk.code.data()), code_size);
            }

            file.close();
            return lib_chunk;
        }

    }
}
