// === File: src/core/display.cpp ==============================================
// AGENT: PURPOSE    — SDL2/WebGL display management for Gource Web
// AGENT: STATUS     — in-progress (2026-01-09)
// =============================================================================

#include "display.h"
#include "sdlapp.h"
#include "renderer.h"
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

SDLAppDisplay display;

SDLAppDisplay::SDLAppDisplay() {
    clear_colour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    viewport_dpi_ratio = glm::vec2(1.0f, 1.0f);
    enable_alpha = false;
    vsync = true;
    resizable = true;
    fullscreen = false;
    multi_sample = 0;
    width = 0;
    height = 0;
    sdl_window = nullptr;
    gl_context = nullptr;
}

SDLAppDisplay::~SDLAppDisplay() {
}

void SDLAppDisplay::setClearColour(glm::vec3 colour) {
    setClearColour(glm::vec4(colour, enable_alpha ? 0.0f : 1.0f));
}

void SDLAppDisplay::setClearColour(glm::vec4 colour) {
    clear_colour = colour;
}

void SDLAppDisplay::enableVsync(bool vsync) {
    this->vsync = vsync;
}

void SDLAppDisplay::enableResize(bool resizable) {
    this->resizable = resizable;
}

void SDLAppDisplay::enableAlpha(bool enable) {
    enable_alpha = enable;
}

void SDLAppDisplay::multiSample(int samples) {
    multi_sample = samples;
}

bool SDLAppDisplay::multiSamplingEnabled() {
    int value;
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &value);
    return value == 1;
}

void SDLAppDisplay::init(std::string window_title, int w, int h, bool fs, int screen) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        throw SDLInitException(SDL_GetError());
    }

#ifdef __EMSCRIPTEN__
    // Get canvas size from browser
    double css_w, css_h;
    emscripten_get_element_css_size("#canvas", &css_w, &css_h);

    if (w == 0) w = (int)css_w;
    if (h == 0) h = (int)css_h;
#endif
    if (w == 0) w = 1280;
    if (h == 0) h = 720;

#ifdef __EMSCRIPTEN__
    // OpenGL ES 3.0 for WebGL 2
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    // OpenGL 3.3 Core for native
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    if (enable_alpha) {
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    }

    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
    if (resizable) flags |= SDL_WINDOW_RESIZABLE;
    if (fs) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#ifndef __EMSCRIPTEN__
    flags |= SDL_WINDOW_ALLOW_HIGHDPI;  // Enable HiDPI on native
#endif

    sdl_window = SDL_CreateWindow(
        window_title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        w, h, flags
    );

    if (!sdl_window) {
        throw SDLInitException(SDL_GetError());
    }

    gl_context = SDL_GL_CreateContext(sdl_window);
    if (!gl_context) {
        throw SDLInitException(SDL_GetError());
    }

#ifndef __EMSCRIPTEN__
    // For Emscripten, browser controls frame timing via requestAnimationFrame
    // SDL_GL_SetSwapInterval requires emscripten_set_main_loop to be active
    SDL_GL_SetSwapInterval(vsync ? 1 : 0);
#endif

    // Get actual GL viewport size (may differ from window size on HiDPI)
    int drawable_w, drawable_h;
    SDL_GL_GetDrawableSize(sdl_window, &drawable_w, &drawable_h);
    width = drawable_w;
    height = drawable_h;

    // Calculate DPI ratio (for HiDPI/Retina displays)
    int window_w, window_h;
    SDL_GetWindowSize(sdl_window, &window_w, &window_h);
    viewport_dpi_ratio.x = (float)drawable_w / (float)window_w;
    viewport_dpi_ratio.y = (float)drawable_h / (float)window_h;

    glViewport(0, 0, width, height);

    fullscreen = fs;

    debugLog("GL context created: %d x %d (DPI ratio: %.2f)", width, height, viewport_dpi_ratio.x);
    debugLog("GL Version: %s", glGetString(GL_VERSION));
    debugLog("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    // Initialize renderer
    renderer().init();
}

void SDLAppDisplay::quit() {
    renderer().shutdown();

    texturemanager.purge();
    shadermanager.purge();
    fontmanager.purge();
    fontmanager.destroy();

    if (gl_context) {
        SDL_GL_DeleteContext(gl_context);
        gl_context = nullptr;
    }
    if (sdl_window) {
        SDL_DestroyWindow(sdl_window);
        sdl_window = nullptr;
    }
}

void SDLAppDisplay::update() {
    SDL_GL_SwapWindow(sdl_window);
}

void SDLAppDisplay::clear() {
    glClearColor(clear_colour.x, clear_colour.y, clear_colour.z, clear_colour.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void SDLAppDisplay::resize(int w, int h) {
    SDL_GL_GetDrawableSize(sdl_window, &width, &height);
    glViewport(0, 0, width, height);
    debugLog("Resized to %d x %d", width, height);
}

void SDLAppDisplay::mode3D(float fov, float znear, float zfar) {
    renderer().mode3D(fov, (float)width / (float)height, znear, zfar);
}

void SDLAppDisplay::mode2D() {
    renderer().mode2D(width, height);
}

void SDLAppDisplay::push2D() {
    renderer().push2D(width, height);
}

void SDLAppDisplay::pop2D() {
    renderer().pop2D();
}

glm::vec4 SDLAppDisplay::currentColour() {
    return renderer().getCurrentColor();
}

glm::vec3 SDLAppDisplay::project(glm::vec3 pos) {
    glm::mat4 model = renderer().getModelView();
    glm::mat4 proj = renderer().getProjection();
    glm::vec4 viewport(0, 0, width, height);

    glm::vec3 win = glm::project(pos, model, proj, viewport);
    win.y = height - win.y;  // Flip Y

    return win;
}

glm::vec3 SDLAppDisplay::unproject(glm::vec2 pos) {
    glm::mat4 model = renderer().getModelView();
    glm::mat4 proj = renderer().getProjection();
    glm::vec4 viewport(0, 0, width, height);

    // Read depth at position
    float depth;
    glReadPixels((int)pos.x, height - (int)pos.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    glm::vec3 win(pos.x, height - pos.y, depth);
    return glm::unProject(win, model, proj, viewport);
}

bool SDLAppDisplay::isFullscreen() const {
    return fullscreen;
}

void SDLAppDisplay::toggleFullscreen() {
#ifdef __EMSCRIPTEN__
    // For web, request fullscreen via Emscripten
    if (!fullscreen) {
        EmscriptenFullscreenStrategy strategy = {};
        strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
        strategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
        strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
        emscripten_enter_soft_fullscreen("#canvas", &strategy);
    } else {
        emscripten_exit_soft_fullscreen();
    }
#else
    // Native fullscreen toggle via SDL
    Uint32 flags = SDL_GetWindowFlags(sdl_window);
    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
        SDL_SetWindowFullscreen(sdl_window, 0);
    } else {
        SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
#endif
    fullscreen = !fullscreen;
}

void SDLAppDisplay::toggleFrameless() {
#ifndef __EMSCRIPTEN__
    // Native frameless/borderless toggle
    Uint32 flags = SDL_GetWindowFlags(sdl_window);
    if (flags & SDL_WINDOW_BORDERLESS) {
        SDL_SetWindowBordered(sdl_window, SDL_TRUE);
    } else {
        SDL_SetWindowBordered(sdl_window, SDL_FALSE);
    }
#endif
    // No-op on web - frameless doesn't apply
}

void SDLAppDisplay::getFullscreenResolution(int& w, int& h) {
    w = width;
    h = height;
}
