// GLEW will include OpenGL.
// ... except that right now the Linux build doesn't have GLEW installed, so
// temporarily use the normal GL headers there instead.
// This should be fine until there's a UI build for non-Windows sytems.
#ifdef WIN32
    #include <GL/glew.h>
#else
    #include <GL/gl.h>
    #include <GL/glu.h>
#endif

#include <cradle/external/clean.hpp>
