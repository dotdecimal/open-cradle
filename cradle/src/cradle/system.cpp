#include <cradle/system.hpp>

#ifdef WIN32
    #include <windows.h>
#endif

namespace cradle {

uint64_t get_total_physical_memory()
{
  #ifdef WIN32
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return statex.ullTotalPhys;
  #else
    return 0;
  #endif
}

uint64_t get_free_physical_memory()
{
  #ifdef WIN32
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return statex.ullAvailPhys;
  #else
    return 0;
  #endif
}

}
