#ifndef IODICIUM_COMMON_OPCODE_H
#define IODICIUM_COMMON_OPCODE_H

#include <cstdint>

enum OpCode : uint8_t {
    // --- Core Operations ---
    OP_RETURN = 0x00,
    OP_CALL = 0x01,   // Calls a function. Operand: <uint8_t arg_count>, <uint16_t address>

    // --- Constant Loading ---
    OP_CONST = 0x02,
    OP_CONST_16 = 0x03,

    // --- I/O ---
    OP_WRITE_OUT = 0x04,
    OP_WRITE_ERR = 0x05,
    OP_FLUSH = 0x06,

    // --- Arithmetic Operations ---
    OP_ADD = 0x07,
    OP_SUBTRACT = 0x08,
    OP_MULTIPLY = 0x09,
    OP_DIVIDE = 0x0A,

    // --- Variable Operations ---
    OP_DEFINE_GLOBAL = 0x0B,
    OP_GET_GLOBAL = 0x0C,
    OP_SET_GLOBAL = 0x0D,
    OP_GET_LOCAL = 0x0E, // Reads a local variable. Operand: <uint8_t slot_index>
    OP_SET_LOCAL = 0x0F, // Writes to a local variable. Operand: <uint8_t slot_index>

    // --- Type Operations ---
    OP_CONVERT = 0x10,
};

#endif //IODICIUM_COMMON_OPCODE_H
