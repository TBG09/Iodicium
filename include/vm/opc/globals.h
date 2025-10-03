#ifndef IODICIUM_VM_OPC_GLOBALS_H
#define IODICIUM_VM_OPC_GLOBALS_H

namespace Iodicium {
    namespace VM {
        class VirtualMachine; // Forward declaration

        // Opcode handler functions for global variables
        bool op_define_global(VirtualMachine& vm);
        bool op_get_global(VirtualMachine& vm);
        bool op_set_global(VirtualMachine& vm);

    }
}

#endif //IODICIUM_VM_OPC_GLOBALS_H
