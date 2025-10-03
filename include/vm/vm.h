#ifndef IODICIUM_VM_VM_H
#define IODICIUM_VM_VM_H

#include <vector>
#include <string>
#include <map>
#include "common/logger.h"
#include "executable/ioe_reader.h"

namespace Iodicium {
    namespace VM {

        // Represents a single frame on the call stack.
        struct CallFrame {
            Executable::Chunk* chunk; // The chunk this frame is executing
            size_t ip;                // The instruction pointer for this frame
        };

        class VirtualMachine {
        public:
            explicit VirtualMachine(Common::Logger& logger, size_t memory_limit = 0);
            void run(Executable::Chunk& chunk);

        private:
            Common::Logger& m_logger;
            size_t m_memory_limit;
            std::vector<std::string> m_stack;
            std::map<std::string, std::string> m_globals;
            std::vector<CallFrame> m_call_stack;

            // Helper methods
            void push(const std::string& value);
            std::string pop();
        };

    }
}

#endif //IODICIUM_VM_VM_H
