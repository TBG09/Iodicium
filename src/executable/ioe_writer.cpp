#include "executable/ioe_writer.h"
#include <fstream>
#include "common/error.h" // Include base error class

namespace Iodicium {
    namespace Executable {

        // File format constants
        const uint32_t IOE_MAGIC_NUMBER = 0x45444F49; // 'IODE'
        const uint8_t IOE_VERSION = 0x01;

        IoeWriter::IoeWriter(Common::Logger& logger) : m_logger(logger) {
            m_logger.debug("IoeWriter constructor called.");
        }

        void IoeWriter::setCode(std::vector<uint8_t> code) {
            m_logger.debug("IoeWriter: Setting code section.");
            m_code_section = std::move(code);
        }

        void IoeWriter::addConstant(const std::string& constant) {
            m_logger.debug("IoeWriter: Adding constant: " + constant);
            m_data_section.push_back(constant);
        }

        void IoeWriter::setImports(const std::vector<std::string>& imports) {
            m_logger.debug("IoeWriter: Setting import section.");
            m_import_section = imports;
        }

        void IoeWriter::writeToFile(const std::string& path) {
            m_logger.debug("IoeWriter: Writing executable to: " + path);
            std::ofstream file(path, std::ios::binary);
            if (!file.is_open()) {
                m_logger.error("IoeWriter: Failed to open file for writing: " + path);
                throw IoeWriterError("Failed to open file for writing: " + path);
            }

            // Write the .iode header
            file.write(reinterpret_cast<const char*>(&IOE_MAGIC_NUMBER), sizeof(IOE_MAGIC_NUMBER));
            file.write(reinterpret_cast<const char*>(&IOE_VERSION), sizeof(IOE_VERSION));

            // Write the import section (import table)
            uint32_t import_count = static_cast<uint32_t>(m_import_section.size());
            file.write(reinterpret_cast<const char*>(&import_count), sizeof(import_count));
            for (const auto& import_path : m_import_section) {
                uint32_t path_length = static_cast<uint32_t>(import_path.length());
                file.write(reinterpret_cast<const char*>(&path_length), sizeof(path_length));
                file.write(import_path.data(), path_length);
            }

            // Write the data section (constant pool)
            uint32_t constant_count = static_cast<uint32_t>(m_data_section.size());
            file.write(reinterpret_cast<const char*>(&constant_count), sizeof(constant_count));
            for (const auto& constant : m_data_section) {
                uint32_t constant_length = static_cast<uint32_t>(constant.length());
                file.write(reinterpret_cast<const char*>(&constant_length), sizeof(constant_length));
                file.write(constant.data(), constant_length);
            }

            // Write the code section (bytecode)
            uint32_t code_size = static_cast<uint32_t>(m_code_section.size());
            file.write(reinterpret_cast<const char*>(&code_size), sizeof(code_size));
            if (!m_code_section.empty()) {
                file.write(reinterpret_cast<const char*>(m_code_section.data()), code_size);
            }

            file.close();
            m_logger.debug("IoeWriter: File closed: " + path);
        }

    }
}
