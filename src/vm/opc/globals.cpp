/*
#include "vm/opcodes.h"
#include "vm/vm.h"

namespace Iodicium {
    namespace VM {

        bool op_define_global(VirtualMachine& vm) {
            // Read the global variable name index from bytecode
            uint8_t name_index = *vm.m_ip++;
            std::string var_name = vm.m_chunk.constants[name_index];
            StackValue value = vm.pop();

            if (vm.m_globals.count(var_name)) {
                vm.getLogger().error("Runtime Error: Variable '" + var_name + "' already defined.");
                return false;
            }
            vm.m_globals[var_name] = value;
            vm.getLogger().debug("VM: Defined global '" + var_name + "' with value: " + value.toString());
            return true;
        }

        bool op_get_global(VirtualMachine& vm) {
            uint8_t name_index = *vm.m_ip++;
            std::string var_name = vm.m_chunk.constants[name_index];

            auto it = vm.m_globals.find(var_name);
            if (it == vm.m_globals.end()) {
                vm.getLogger().error("Runtime Error: Undefined variable '" + var_name + "'.");
                return false;
            }
            vm.push(it->second);
            vm.getLogger().debug("VM: Got global '" + var_name + "' value: " + it->second.toString());
            return true;
        }

        bool op_set_global(VirtualMachine& vm) {
            uint8_t name_index = *vm.m_ip++;
            std::string var_name = vm.m_chunk.constants[name_index];
            StackValue value = vm.pop(); // The new value is on top of the stack

            if (vm.m_globals.count(var_name) == 0) {
                vm.getLogger().error("Runtime Error: Cannot set undefined variable '" + var_name + "'.");
                return false;
            }
            vm.m_globals[var_name] = value;
            vm.getLogger().debug("VM: Set global '" + var_name + "' to value: " + value.toString());
            return true;
        }

    }
}
*/