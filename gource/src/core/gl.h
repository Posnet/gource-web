// === File: src/core/gl.h =====================================================
// AGENT: PURPOSE    — WebGL 2.0 / OpenGL ES 3.0 headers for Gource Web
// AGENT: OWNER      — agent
// AGENT: STATUS     — in-progress (2026-01-09)
// =============================================================================

#ifndef CORE_GL_H
#define CORE_GL_H

// Platform-specific GL headers
#ifdef __EMSCRIPTEN__
    // Emscripten provides OpenGL ES 3.0
    #include <GLES3/gl3.h>

    // Types not in OpenGL ES but used in legacy code
    #ifndef GLdouble
    typedef double GLdouble;
    #endif
    #ifndef GL_CLAMP
    #define GL_CLAMP GL_CLAMP_TO_EDGE
    #endif

    // GL_TEXTURE_2D enable/disable is not valid in OpenGL ES 3.0
    // Define it so code compiles, but make enable/disable no-ops
    #ifndef GL_TEXTURE_2D
    #define GL_TEXTURE_2D 0x0DE1
    #endif
    #define GL_TEXTURE_2D_LEGACY 0x0DE1

    // Wrapper to skip GL_TEXTURE_2D enable/disable in WebGL
    inline void glEnableCompat(GLenum cap) {
        if (cap != GL_TEXTURE_2D_LEGACY) glEnable(cap);
    }
    inline void glDisableCompat(GLenum cap) {
        if (cap != GL_TEXTURE_2D_LEGACY) glDisable(cap);
    }
    #define glEnable(cap) glEnableCompat(cap)
    #define glDisable(cap) glDisableCompat(cap)
#elif defined(__APPLE__)
    // macOS OpenGL (for native testing)
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>

    // GLdouble available on macOS but may need explicit typedef
    #ifndef GLdouble
    typedef double GLdouble;
    #endif
#else
    // Other platforms - assume desktop OpenGL 3.3+
    #include <GL/gl.h>
    #include <GL/glext.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Enable experimental GLM extensions
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/projection.hpp>

// Error string helper
inline const char* glErrorString(GLenum error) {
    switch (error) {
        case GL_NO_ERROR: return "GL_NO_ERROR";
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        default: return "UNKNOWN_ERROR";
    }
}

#define gluErrorString(e) glErrorString(e)

// Compatibility: GL_QUADS is not available in OpenGL ES / WebGL
// The Renderer class handles conversion to triangles
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif

#ifdef DEBUG
    #include "logger.h"
    #define glCheckError() { \
        GLenum gl_error = glGetError(); \
        if(gl_error != GL_NO_ERROR) { \
            warnLog("GL error %s at %s:%d", glErrorString(gl_error), __FILE__, __LINE__); \
        } \
    }
#else
    #define glCheckError()
#endif

#endif
