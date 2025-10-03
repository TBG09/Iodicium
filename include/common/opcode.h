#ifndef IODICIUM_COMMON_OPCODE_H
#define IODICIUM_COMMON_OPCODE_H

#include <cstdint>

// Enum for our bytecode instruction set (opcodes)
enum OpCode : uint8_t {
    // --- Core Operations ---
    OP_RETURN = 0x00, // Ends execution of the current function

    // --- Constant Loading ---
    OP_CONST = 0x01,      // Loads a constant from the data section. Operand: <uint8_t constant_index>
    OP_CONST_16 = 0x02,   // Loads a constant. Operand: <uint16_t constant_index> (future use)

    // --- I/O ---
    OP_WRITE_OUT = 0x03,  // Pops a string and writes it to stdout
    OP_WRITE_ERR = 0x04,  // Pops a string and writes it to stderr
    OP_FLUSH = 0x05,      // Flushes stdout and stderr

    // --- Arithmetic Operations ---
    OP_ADD = 0x06,
    OP_SUBTRACT = 0x07,
    OP_MULTIPLY = 0x08,
    OP_DIVIDE = 0x09,

    // --- Global Variable Operations ---
    OP_DEFINE_GLOBAL = 0x0A, // Defines a global variable. Operand: <uint8_t name_index>
    OP_GET_GLOBAL = 0x0B,    // Gets a global variable. Operand: <uint8_t name_index>
    OP_SET_GLOBAL = 0x0C,    // Sets a global variable. Operand: <uint8_t name_index>

    // --- Module Operations ---
    OP_EXECUTE_EXTERNAL = 0x0D, // Executes a function from an imported module. Operand: <uint8_t import_table_index>

    // Add more opcodes here as the language grows...
};

#endif //IODICIUM_COMMON_OPCODE_H
