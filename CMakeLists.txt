cmake_minimum_required(VERSION 4.0)
project(Iodicium)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)


# --- Add cppParse sub-library (it's a separate dependency) ---
add_subdirectory(cppParse)


# --- Define the main executable ---

# Create a list of all source files for the executable
set(IODICIUM_SOURCES
    src/main.cpp
    src/codeparser/ast.cpp
    src/codeparser/lexer.cpp
    src/codeparser/parser.cpp
    src/codeparser/tokenizer.cpp
    src/codeparser/expression.cpp
    src/codeparser/stmt/function.cpp # New: Function statement implementation
    src/codeparser/types/int.cpp
    src/codeparser/types/string.cpp
    src/compiler/codegen.cpp
    src/compiler/compiler.cpp
    src/compiler/semantics.cpp
    src/vm/vm.cpp
    src/vm/library_loader.cpp
    src/vm/value.cpp
    src/common/dialog.cpp
    src/common/logger.cpp
    src/executable/ioe_reader.cpp
    src/executable/ioe_writer.cpp
    src/executable/iodl_reader.cpp # New: For reading library files
    src/executable/iodl_writer.cpp
)

# Create the executable from the source list
add_executable(Iodicium ${IODICIUM_SOURCES})


# --- Configure linking and include paths ---

# Point the main executable to the new, centralized include directory.
# The include paths for sub-libraries are handled automatically by target_link_libraries.
target_include_directories(Iodicium PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(Iodicium PRIVATE ${CMAKE_SOURCE_DIR}/src/common)
target_include_directories(Iodicium PRIVATE ${CMAKE_SOURCE_DIR}/src/executable)


# Set the minimum Windows version to Vista to unlock modern APIs like TaskDialogIndirect
if(WIN32)
    target_compile_definitions(Iodicium PRIVATE _WIN32_WINNT=0x0600)
endif()

target_link_libraries(Iodicium PRIVATE
    cppParse
)

# On Windows, link the final executable against the Common Controls library
if(WIN32)
    target_link_libraries(Iodicium PRIVATE comctl32)
endif()

# --- Embed Manifest on Windows ---
if(WIN32)
    find_program(MT_EXECUTABLE mt.exe)

    if(NOT MT_EXECUTABLE)
        message(STATUS "mt.exe not found in PATH. Searching Windows Kits...")
        file(GLOB SDK_VERSIONS "C:/Program Files (x86)/Windows Kits/10/bin/*")
        
        set(TARGET_ARCH_FOLDER "")
        if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "AMD64")
            set(TARGET_ARCH_FOLDER "x64")
        elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86")
            set(TARGET_ARCH_FOLDER "x86")
        elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "ARM64")
            set(TARGET_ARCH_FOLDER "arm64")
        endif()

        if(TARGET_ARCH_FOLDER)
            list(REVERSE SDK_VERSIONS) # Check newest versions first
            foreach(VERSION_PATH ${SDK_VERSIONS})
                if(IS_DIRECTORY ${VERSION_PATH} AND EXISTS "${VERSION_PATH}/${TARGET_ARCH_FOLDER}/mt.exe")
                    set(MT_EXECUTABLE "${VERSION_PATH}/${TARGET_ARCH_FOLDER}/mt.exe")
                    message(STATUS "Found mt.exe at: ${MT_EXECUTABLE}")
                    break()
                endif()
            endforeach()
        endif()
    endif()

    if(NOT MT_EXECUTABLE)
        message(FATAL_ERROR "mt.exe could not be found. Please ensure the Windows SDK is installed.")
    else()
        add_custom_command(
            TARGET Iodicium POST_BUILD
            COMMAND ${MT_EXECUTABLE} -manifest ${CMAKE_SOURCE_DIR}/app.manifest -outputresource:$<TARGET_FILE:Iodicium>;#1
            COMMENT "Embedding application manifest..."
        )
    endif()
endif()
