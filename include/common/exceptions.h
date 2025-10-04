#ifndef IODICIUM_COMMON_EXCEPTIONS_H
#define IODICIUM_COMMON_EXCEPTIONS_H

#include "common/error.h"
#include "codeparser/tokenizer.h"
#include <string>

namespace Iodicium {
    namespace Common {

        // Base class for all specific compiler/runtime exceptions
        class CompilerException : public IodiciumError {
        public:
            CompilerException(const std::string& message, int line = -1, int column = -1)
                : IodiciumError(message, line, column) {}
        };

        // --- Parser Specific Exceptions ---
        class UnexpectedTokenException : public CompilerException {
        public:
            UnexpectedTokenException(const std::string& expected, const Codeparser::Token& found)
                : CompilerException(
                    "Expected '" + expected + "', but found '" + found.lexeme + "' (Type: " + std::to_string(static_cast<int>(found.type)) + ").",
                    found.line, found.column) {}
        };

        class MissingTokenException : public CompilerException {
        public:
            MissingTokenException(const std::string& expected, const Codeparser::Token& context_token)
                : CompilerException(
                    "Expected '" + expected + "', but reached end of file or unexpected token.",
                    context_token.line, context_token.column) {}
        };

        // --- Semantic Specific Exceptions ---
        class UndefinedSymbolException : public CompilerException {
        public:
            UndefinedSymbolException(const std::string& symbol_name, const Codeparser::Token& context_token)
                : CompilerException(
                    "Undefined symbol '" + symbol_name + "'.",
                    context_token.line, context_token.column) {}
        };

        class RedeclarationException : public CompilerException {
        public:
            RedeclarationException(const std::string& symbol_name, const Codeparser::Token& context_token)
                : CompilerException(
                    "Symbol '" + symbol_name + "' already declared in this scope.",
                    context_token.line, context_token.column) {}
        };

        // --- Runtime Specific Exceptions ---
        class StackUnderflowException : public CompilerException {
        public:
            StackUnderflowException(const std::string& message)
                : CompilerException(message) {}
        };

        // Add more specific exceptions as needed...

    }
}

#endif //IODICIUM_COMMON_EXCEPTIONS_H
