#include "executable/iodl_writer.h"
#include <fstream>

namespace Iodicium {
    namespace Executable {

        // File format constants
        const uint32_t IODL_MAGIC_NUMBER = 0x4C444F49; // 'IODL'
        const uint8_t IODL_VERSION = 0x01;

        IodlWriter::IodlWriter(Common::Logger& logger) : m_logger(logger) {
            m_logger.debug("IodlWriter constructor called.");
        }

        void IodlWriter::setCode(std::vector<uint8_t> code) {
            m_code_section = std::move(code);
        }

        void IodlWriter::addConstant(const std::string& constant) {
            m_data_section.push_back(constant);
        }

        void IodlWriter::setExports(const std::map<std::string, size_t>& exports) {
            m_export_section = exports;
        }

        void IodlWriter::writeToFile(const std::string& path) {
            m_logger.debug("IodlWriter: Writing library to: " + path);
            std::ofstream file(path, std::ios::binary);
            if (!file.is_open()) {
                throw IodlWriterError("Failed to open file for writing: " + path);
            }

            // Write the .iodl header
            file.write(reinterpret_cast<const char*>(&IODL_MAGIC_NUMBER), sizeof(IODL_MAGIC_NUMBER));
            file.write(reinterpret_cast<const char*>(&IODL_VERSION), sizeof(IODL_VERSION));

            // Write the export section (export table)
            uint32_t export_count = static_cast<uint32_t>(m_export_section.size());
            file.write(reinterpret_cast<const char*>(&export_count), sizeof(export_count));
            for (const auto& [name, ip] : m_export_section) {
                uint32_t name_length = static_cast<uint32_t>(name.length());
                file.write(reinterpret_cast<const char*>(&name_length), sizeof(name_length));
                file.write(name.data(), name_length);
                uint64_t ip_64 = static_cast<uint64_t>(ip); // Use a fixed-size integer for IP
                file.write(reinterpret_cast<const char*>(&ip_64), sizeof(ip_64));
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
        }

    }
}
