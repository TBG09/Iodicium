# Define the library target from all its source files
add_library(cppToml
    src/parser.cpp
    src/ast.cpp
    src/lexer.cpp
    src/nesting.cpp
    src/tables.cpp
    src/types/TomlArray.cpp
    src/types/TomlBoolean.cpp
    src/types/TomlDateTime.cpp
    src/types/TomlFloat.cpp
    src/types/TomlInteger.cpp
    src/types/TomlString.cpp
)

# Make the headers in the 'include' directory public, so that other
# targets (like the main Iodicium executable) can find them.
target_include_directories(cppToml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
