/*
#include "vm/opcodes.h"
#include "vm/vm.h"
#include <functional>

namespace Iodicium {
    namespace VM {

        template<typename Op>
        bool binary_op(VirtualMachine& vm, Op op, const std::string& op_name) {
            StackValue b = vm.pop();
            StackValue a = vm.pop();

            if (a.isNumber() && b.isNumber()) {
                double result = op(a.asNumber(), b.asNumber());
                vm.getLogger().debug("VM: Performed " + op_name + " on numbers: " + a.toString() + ", " + b.toString());
                vm.push(Value(result));
            } else if (a.isString() && b.isString() && op_name == "ADD") {
                std::string result = a.asString() + b.asString();
                vm.getLogger().debug("VM: Performed string concatenation: " + a.toString() + ", " + b.toString());
                vm.push(Value(result));
            } else {
                vm.getLogger().error("Runtime Error: Unsupported operands for " + op_name + ": " + a.toString() + ", " + b.toString());
                return false;
            }
        }

        bool op_add(VirtualMachine& vm) {
            return binary_op(vm, std::plus<double>(), "ADD");
        }

        bool op_subtract(VirtualMachine& vm) {
            return binary_op(vm, std::minus<double>(), "SUBTRACT");
        }

        bool op_multiply(VirtualMachine& vm) {
            return binary_op(vm, std::multiplies<double>(), "MULTIPLY");
        }

        bool op_divide(VirtualMachine& vm) {
            StackValue b = vm.pop();
            StackValue a = vm.pop();

            if (a.isNumber() && b.isNumber()) {
                if (b.asNumber() == 0) {
                    vm.getLogger().error("Runtime Error: Division by zero.");
                    return false;
                }
                double result = a.asNumber() / b.asNumber();
                vm.getLogger().debug("VM: Performed DIVIDE on numbers: " + a.toString() + ", " + b.toString());
                vm.push(Value(result));
            } else {
                vm.getLogger().error("Runtime Error: Unsupported operands for DIVIDE: " + a.toString() + ", " + b.toString());
                return false;
            }
        }

    }
}
*/