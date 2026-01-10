// === File: src/core/sdlapp.h ==================================================
// AGENT: PURPOSE    — SDL application framework header for Emscripten/WebGL
// AGENT: STATUS     — in-progress (2026-01-09)
// =============================================================================

#ifndef SDLAPP_H
#define SDLAPP_H

#include "SDL.h"
#include "gl.h"

#include <string>
#include <exception>
#include <vector>

extern std::string gSDLAppConfDir;
extern std::string gSDLAppResourceDir;
extern std::string gSDLAppPathSeparator;
extern std::string gSDLAppTitle;
extern std::string gSDLAppExec;

void SDLAppInfo(std::string msg);
void SDLAppQuit(std::string error);
void SDLAppInit(std::string apptitle, std::string execname, std::string exepath = "");
std::string SDLAppAddSlash(std::string path);
void SDLAppParseArgs(int argc, char* argv[], int* xres, int* yres, bool* fullscreen, std::vector<std::string>* otherargs = nullptr);

class SDLAppException : public std::exception {
protected:
    std::string message;
    bool showhelp;

public:
    SDLAppException(const char* str, ...) : showhelp(false) {
        va_list vl;
        char msg[65536];
        va_start(vl, str);
        vsnprintf(msg, 65536, str, vl);
        va_end(vl);
        message = std::string(msg);
    }

    SDLAppException(std::string message) : showhelp(false), message(message) {}
    ~SDLAppException() throw() {}

    bool showHelp() const { return showhelp; }
    void setShowHelp(bool showhelp) { this->showhelp = showhelp; }

    virtual const char* what() const throw() { return message.c_str(); }
};

class SDLApp {
    int return_code;

protected:
    int min_delta_msec;
    bool appFinished;

    void stop(int return_code);

public:
    // Made public for Emscripten main loop callback
    void updateFramerate();
    virtual bool handleEvent(SDL_Event& event);
    float fps;
    int frame_count;
    int fps_updater;

    SDLApp();
    virtual ~SDLApp() {}

    int run();

    static bool getClipboardText(std::string& text);
    static void setClipboardText(const std::string& text);

    virtual void resize(int width, int height) {}
    virtual void update(float t, float dt) {}
    virtual void init() {}
    virtual void logic(float t, float dt) {}
    virtual void draw(float t, float dt) {}
    virtual void quit() { appFinished = true; }

    virtual void mouseMove(SDL_MouseMotionEvent* e) {}
    virtual void mouseClick(SDL_MouseButtonEvent* e) {}
    virtual void keyPress(SDL_KeyboardEvent* e) {}
    virtual void textInput(SDL_TextInputEvent* e) {}
    virtual void textEdit(SDL_TextEditingEvent* e) {}
    virtual void mouseWheel(SDL_MouseWheelEvent* e) {}

    int returnCode();
    bool isFinished();
};

#endif
