#ifndef CRADLE_EXTERNAL_LIBPQ_HPP
#define CRADLE_EXTERNAL_LIBPQ_HPP

#include <libpq-fe.h>

#ifdef _MSC_VER

#pragma comment (lib, "advapi32.lib")
#pragma comment (lib, "shell32.lib")
#pragma comment (lib, "wsock32.lib")
#pragma comment (lib, "secur32.lib")

#endif

#endif
