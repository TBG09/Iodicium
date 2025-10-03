#ifndef IODICIUM_EXECUTABLE_EXPORT_H
#define IODICIUM_EXECUTABLE_EXPORT_H

#if defined(_WIN32)
    #if defined(IOD_EXECUTABLE_BUILD_DLL)
        // We are building this library, so export the symbols
        #define IOD_EXECUTABLE_API __declspec(dllexport)
    #else
        // We are using this library, so import the symbols (or nothing if building statically)
        #define IOD_EXECUTABLE_API
    #endif
#else
    // On non-Windows platforms, we don't need these macros
    #define IOD_EXECUTABLE_API
#endif

#endif //IODICIUM_EXECUTABLE_EXPORT_H
