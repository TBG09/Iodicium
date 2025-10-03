/*
#include "vm/opcodes.h"
#include "vm/vm.h"

namespace Iodicium {
    namespace VM {

        bool op_return(VirtualMachine& vm) {
            // In the new VM, OP_RETURN is handled directly in the main loop
            // to manage the call stack. This function is obsolete.
            vm.m_logger.debug("Executing OP_RETURN");
            return false; // Returning false stops the VM loop
        }

    }
}
*/