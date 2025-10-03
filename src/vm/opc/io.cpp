/*
#include "vm/opcodes.h"
#include "vm/vm.h"
#include "vm/value.h"
#include <iostream>
#include <sstream>

namespace Iodicium {
    namespace VM {

        // This function is now obsolete. OP_CONST is handled in the main VM loop.
        bool op_const(VirtualMachine& vm) {
            uint8_t const_index = *vm.m_ip++;

            const std::string& constant_str = vm.m_chunk.constants[const_index];
            Value constant_value;

            // Attempt to parse as a double first
            try {
                size_t pos;
                double num = std::stod(constant_str, &pos);
                // Check if the entire string was consumed
                if (pos == constant_str.length()) {
                    constant_value = Value(num);
                } else {
                    // Not a valid number, treat as a string
                    constant_value = Value(constant_str);
                }
            } catch (const std::invalid_argument&) {
                // Not a number, treat as a string
                constant_value = Value(constant_str);
            } catch (const std::out_of_range&) {
                // Number out of range for double, treat as string
                constant_value = Value(constant_str);
            }

            vm.m_logger.debug("Executing OP_CONST: loading constant '" + constant_str + "' (as " + constant_value.getTypeString() + ") at index " + std::to_string(const_index));

            vm.push(constant_value);
            return true;
        }

        // This function is now obsolete. OP_PRINTLN is handled in the main VM loop.
        bool op_println(VirtualMachine& vm) {
            Value value = vm.pop();
            std::stringstream ss;
            ss << value;
            std::cout << ss.str() << std::endl;
            vm.m_logger.debug("Executing OP_PRINTLN: printing value '" + value.getTypeString() + "(" + ss.str() + ")'"); // Updated logging
            return true;
        }

    }
}
*/