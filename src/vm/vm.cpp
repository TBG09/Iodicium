#include "vm/vm.h"
#include "common/opcode.h"
#include <iostream>
#include <iomanip> // For std::setw

enum DataType : uint8_t {
    UNKNOWN,
    NIL,
    BOOL,
    INT,
    DOUBLE,
    STRING,
    FUNCTION
};

namespace Iodicium {
    namespace VM {

        void printStack(const std::vector<std::string>& stack) {
            std::cout << "          [ ";
            for (const auto& val : stack) {
                std::cout << val << " ";
            }
            std::cout << "]" << std::endl;
        }

        void disassembleInstruction(const Executable::Chunk* chunk, size_t ip) {
            std::cout << std::setw(4) << std::setfill('0') << ip << " ";
            uint8_t instruction = chunk->code[ip];

            switch (instruction) {
                case OP_RETURN: std::cout << "OP_RETURN" << std::endl; break;
                case OP_CALL: std::cout << "OP_CALL" << std::endl; break;
                case OP_CONST: std::cout << "OP_CONST" << std::endl; break;
                case OP_WRITE_OUT: std::cout << "OP_WRITE_OUT" << std::endl; break;
                case OP_WRITE_ERR: std::cout << "OP_WRITE_ERR" << std::endl; break;
                case OP_FLUSH: std::cout << "OP_FLUSH" << std::endl; break;
                case OP_ADD: std::cout << "OP_ADD" << std::endl; break;
                case OP_SUBTRACT: std::cout << "OP_SUBTRACT" << std::endl; break;
                case OP_MULTIPLY: std::cout << "OP_MULTIPLY" << std::endl; break;
                case OP_DIVIDE: std::cout << "OP_DIVIDE" << std::endl; break;
                case OP_DEFINE_GLOBAL: std::cout << "OP_DEFINE_GLOBAL" << std::endl; break;
                case OP_GET_GLOBAL: std::cout << "OP_GET_GLOBAL" << std::endl; break;
                case OP_SET_GLOBAL: std::cout << "OP_SET_GLOBAL" << std::endl; break;
                case OP_GET_LOCAL: std::cout << "OP_GET_LOCAL" << std::endl; break;
                case OP_SET_LOCAL: std::cout << "OP_SET_LOCAL" << std::endl; break;
                case OP_CONVERT: std::cout << "OP_CONVERT" << std::endl; break;
                default: std::cout << "Unknown Opcode: " << (int)instruction << std::endl; break;
            }
        }

        VirtualMachine::VirtualMachine(Common::Logger& logger, size_t memory_limit) 
            : m_logger(logger), m_memory_limit(memory_limit) {}

        void VirtualMachine::run(Executable::Chunk& main_chunk) {
            m_logger.info("Initializing Iodicium VM...");

            m_call_stack.clear();
            m_call_stack.push_back({&main_chunk, 0, 0});

            m_stack.clear();
            m_globals.clear();

            while (true) {
                CallFrame& frame = m_call_stack.back();

                if (m_logger.getLevel() == Common::LogLevel::Debug) {
                    printStack(m_stack);
                    disassembleInstruction(frame.chunk, frame.ip);
                }

                uint8_t instruction = frame.chunk->code[frame.ip++];

                switch (instruction) {
                    case OP_RETURN: {
                        std::string return_value = pop();
                        m_call_stack.pop_back();
                        if (m_call_stack.empty()) {
                            return;
                        }
                        m_stack.resize(m_call_stack.back().stack_base);
                        push(return_value);
                        break;
                    }
                    case OP_CALL: {
                        uint8_t arg_count = frame.chunk->code[frame.ip++];
                        uint8_t high_byte = frame.chunk->code[frame.ip];
                        uint8_t low_byte = frame.chunk->code[frame.ip + 1];
                        uint16_t address = (high_byte << 8) | low_byte;
                        frame.ip += 2;

                        m_logger.debug("  [VM_CALL] Arg count: " + std::to_string(arg_count));
                        m_logger.debug("  [VM_CALL] Raw address bytes: " + std::to_string(high_byte) + ", " + std::to_string(low_byte));
                        m_logger.debug("  [VM_CALL] Jumping to address: " + std::to_string(address));
                        
                        CallFrame new_frame = {frame.chunk, address, m_stack.size() - arg_count};
                        m_call_stack.push_back(new_frame);
                        break;
                    }
                    case OP_CONST: {
                        uint8_t const_index = frame.chunk->code[frame.ip++];
                        push(frame.chunk->constants[const_index]);
                        break;
                    }
                    case OP_WRITE_OUT: { std::cout << pop(); break; }
                    case OP_WRITE_ERR: { std::cerr << pop(); break; }
                    case OP_FLUSH: { std::cout.flush(); std::cerr.flush(); break; }
                    case OP_ADD: {
                        std::string b = pop();
                        std::string a = pop();
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
                        m_globals[frame.chunk->constants[name_index]] = m_stack.back();
                        break;
                    }
                    case OP_GET_LOCAL: {
                        uint8_t slot_index = frame.chunk->code[frame.ip++];
                        push(m_stack[frame.stack_base + slot_index]);
                        break;
                    }
                    case OP_SET_LOCAL: {
                        uint8_t slot_index = frame.chunk->code[frame.ip++];
                        m_stack[frame.stack_base + slot_index] = m_stack.back(); // Peek
                        break;
                    }
                    case OP_CONVERT: {
                        DataType target_type = (DataType)frame.chunk->code[frame.ip++];
                        std::string value = pop();
                        try {
                            switch (target_type) {
                                case DataType::INT: {
                                    double num_val = std::stod(value);
                                    push(std::to_string((int)num_val));
                                    break;
                                }
                                case DataType::DOUBLE: {
                                    double double_val = std::stod(value);
                                    push(std::to_string(double_val));
                                    break;
                                }
                                case DataType::STRING: {
                                    push(value);
                                    break;
                                }
                                default:
                                    throw std::runtime_error("Unsupported conversion type requested in VM.");
                            }
                        } catch (const std::invalid_argument&) {
                            throw std::runtime_error("Cannot convert '" + value + "' to the requested numeric type.");
                        }
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
            if (m_stack.empty()) throw std::runtime_error("VM Stack Underflow");
            std::string value = m_stack.back();
            m_stack.pop_back();
            return value;
        }

    }
}
