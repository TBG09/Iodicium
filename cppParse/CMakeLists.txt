cmake_minimum_required(VERSION 3.10)

project(cppParse)

# Explicitly list all source files for reliability
add_library(cppParse STATIC
    src/parser.cpp
    src/argument.cpp
    src/help_formatter.cpp
)

# Expose the include directory publicly
target_include_directories(cppParse PUBLIC include)
