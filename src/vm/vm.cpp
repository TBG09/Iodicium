#include "vm/vm.h"
#include "common/opcode.h"
#include <iostream>

namespace Iodicium {
    namespace VM {

        VirtualMachine::VirtualMachine(Common::Logger& logger, size_t memory_limit) 
            : m_logger(logger), m_library_loader(logger), m_memory_limit(memory_limit) {}

        void VirtualMachine::run(Executable::Chunk& main_chunk, const std::string& base_path) {
            m_logger.info("Initializing Iodicium VM...");

            // Prepare all imported libraries and functions before execution begins.
            m_library_loader.prepare(main_chunk, base_path);

            // Set up the first CallFrame for the main script body.
            m_call_stack.clear();
            m_call_stack.push_back({&main_chunk, 0});

            m_stack.clear();
            m_globals.clear();

            while (true) {
                // Get the current frame
                CallFrame& frame = m_call_stack.back();

                // Fetch and execute the next instruction
                uint8_t instruction = frame.chunk->code[frame.ip++];

                switch (instruction) {
                    case OP_RETURN: {
                        m_logger.debug("VM: Executing OP_RETURN.");
                        m_call_stack.pop_back();
                        if (m_call_stack.empty()) {
                            m_logger.info("Execution finished.");
                            return; // End of program
                        }
                        break; // Continue execution in the previous frame
                    }
                    case OP_CONST: {
                        uint8_t const_index = frame.chunk->code[frame.ip++];
                        push(frame.chunk->constants[const_index]);
                        break;
                    }
                    case OP_WRITE_OUT: {
                        std::cout << pop();
                        break;
                    }
                    case OP_WRITE_ERR: {
                        std::cerr << pop();
                        break;
                    }
                    case OP_FLUSH: {
                        std::cout.flush();
                        std::cerr.flush();
                        break;
                    }
                    case OP_ADD: {
                        std::string b = pop();
                        std::string a = pop();
                        // Basic string concatenation or number addition
                        try {
                            double result = std::stod(a) + std::stod(b);
                            push(std::to_string(result));
                        } catch (const std::invalid_argument&) {
                            push(a + b);
                        }
                        break;
                    }
                    case OP_DEFINE_GLOBAL: {
                        uint8_t name_index = frame.chunk->code[frame.ip++];
                        m_globals[frame.chunk->constants[name_index]] = pop();
                        break;
                    }
                    case OP_GET_GLOBAL: {
                        uint8_t name_index = frame.chunk->code[frame.ip++];
                        push(m_globals[frame.chunk->constants[name_index]]);
                        break;
                    }
                    case OP_SET_GLOBAL: {
                        uint8_t name_index = frame.chunk->code[frame.ip++];
                        m_globals[frame.chunk->constants[name_index]] = m_stack.back(); // Peek, don't pop
                        break;
                    }
                    case OP_EXECUTE_EXTERNAL: {
                        m_logger.debug("VM: Executing OP_EXECUTE_EXTERNAL.");
                        uint8_t ordinal = frame.chunk->code[frame.ip++];
                        const LoadedFunction& func = m_library_loader.getFunction(ordinal);
                        
                        // Push a new call frame for the external function
                        CallFrame new_frame = {&func.library->code_chunk, func.ip};
                        m_call_stack.push_back(new_frame);
                        break;
                    }
                    default: {
                        m_logger.error("Unknown opcode: " + std::to_string(instruction));
                        return;
                    }
                }
            }
        }

        void VirtualMachine::push(const std::string& value) {
            m_stack.push_back(value);
        }

        std::string VirtualMachine::pop() {
            std::string value = m_stack.back();
            m_stack.pop_back();
            return value;
        }

    }
}
