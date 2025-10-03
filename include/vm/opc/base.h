#ifndef IODICIUM_VM_OPC_BASE_H
#define IODICIUM_VM_OPC_BASE_H

namespace Iodicium {
    namespace VM {
        class VirtualMachine; // Forward declaration

        // Opcode handler functions
        bool op_return(VirtualMachine& vm);

    }
}

#endif //IODICIUM_VM_OPC_BASE_H
