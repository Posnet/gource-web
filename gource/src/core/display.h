// === File: src/core/display.h ================================================
// AGENT: PURPOSE    — SDL2/WebGL display management header
// AGENT: STATUS     — in-progress (2026-01-09)
// =============================================================================

#ifndef SDLAPP_DISPLAY_H
#define SDLAPP_DISPLAY_H

#include "gl.h"
#include "SDL.h"
#include "shader.h"
#include "logger.h"
#include "texture.h"
#include "fxfont.h"

#include <string>
#include <exception>

class SDLInitException : public std::exception {
protected:
    std::string error;
public:
    SDLInitException(const std::string& error) : error(error) {}
    virtual ~SDLInitException() throw() {}
    virtual const char* what() const throw() { return error.c_str(); }
};

class SDLAppDisplay {
    bool enable_alpha;
    bool resizable;
    bool fullscreen;
    bool vsync;
    int multi_sample;

public:
    int width, height;

    SDL_Window* sdl_window;
    SDL_GLContext gl_context;

    glm::vec4 clear_colour;
    glm::vec2 viewport_dpi_ratio;  // HiDPI/Retina scaling ratio

    SDLAppDisplay();
    ~SDLAppDisplay();

    void init(std::string window_title, int width, int height, bool fullscreen, int screen = -1);
    void quit();

    void getFullscreenResolution(int& width, int& height);
    void toggleFullscreen();
    void toggleFrameless();
    bool isFullscreen() const;

    void resize(int width, int height);
    void update();
    void clear();

    void setClearColour(glm::vec3 colour);
    void setClearColour(glm::vec4 colour);

    void enableVsync(bool vsync);
    void enableAlpha(bool enable);
    void enableResize(bool resizable);
    void multiSample(int sample);
    bool multiSamplingEnabled();

    void mode3D(float fov, float znear, float zfar);
    void mode2D();
    void push2D();
    void pop2D();

    glm::vec4 currentColour();
    glm::vec3 project(glm::vec3 pos);
    glm::vec3 unproject(glm::vec2 pos);
};

extern SDLAppDisplay display;

#endif
