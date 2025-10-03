#ifndef IODICIUM_VM_OPC_IO_H
#define IODICIUM_VM_OPC_IO_H

namespace Iodicium {
    namespace VM {
        class VirtualMachine; // Forward declaration

        // Opcode handler functions
        bool op_const(VirtualMachine& vm);
        bool op_println(VirtualMachine& vm);

    }
}

#endif //IODICIUM_VM_OPC_IO_H
