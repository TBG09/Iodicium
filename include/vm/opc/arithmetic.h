#ifndef IODICIUM_VM_OPC_ARITHMETIC_H
#define IODICIUM_VM_OPC_ARITHMETIC_H

namespace Iodicium {
    namespace VM {
        class VirtualMachine; // Forward declaration

        // Opcode handler functions for arithmetic operations
        bool op_add(VirtualMachine& vm);
        bool op_subtract(VirtualMachine& vm);
        bool op_multiply(VirtualMachine& vm);
        bool op_divide(VirtualMachine& vm);

    }
}

#endif //IODICIUM_VM_OPC_ARITHMETIC_H
