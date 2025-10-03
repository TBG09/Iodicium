#ifndef IODICIUM_COMMON_EXPORT_H
#define IODICIUM_COMMON_EXPORT_H

#if defined(_WIN32)
    #if defined(IOD_COMMON_BUILD_DLL)
        #define IOD_COMMON_API __declspec(dllexport)
    #else
        #define IOD_COMMON_API
    #endif
#else
    #define IOD_COMMON_API
#endif

#endif //IODICIUM_COMMON_EXPORT_H
