// === File: src/core/sdlapp.cpp ================================================
// AGENT: PURPOSE    — SDL application framework for Emscripten/WebGL and Native
// AGENT: STATUS     — in-progress (2026-01-09)
// =============================================================================

#include "sdlapp.h"
#include "display.h"
#include "logger.h"

#include <cstdlib>
#include <filesystem>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

namespace fs = std::filesystem;

std::string gSDLAppResourceDir;
std::string gSDLAppConfDir;
std::string gSDLAppPathSeparator = "/";
std::string gSDLAppTitle = "Gource";
std::string gSDLAppExec = "gource";

std::string SDLAppAddSlash(std::string path) {
    if (path.size() && path[path.size() - 1] != '/') {
        path += '/';
    }
    return path;
}

void SDLAppInfo(std::string msg) {
    printf("%s\n", msg.c_str());
}

void SDLAppQuit(std::string error) {
    fprintf(stderr, "%s: %s\n", gSDLAppExec.c_str(), error.c_str());
#ifdef __EMSCRIPTEN__
    emscripten_force_exit(1);
#else
    exit(1);
#endif
}

bool SDLApp::getClipboardText(std::string& text) {
    char* clipboard_text = SDL_GetClipboardText();
    if (clipboard_text != nullptr) {
        text = std::string(clipboard_text);
        SDL_free(clipboard_text);
        return true;
    }
    text.clear();
    return false;
}

void SDLApp::setClipboardText(const std::string& text) {
    SDL_SetClipboardText(text.c_str());
}

void SDLAppInit(std::string apptitle, std::string execname, std::string exepath) {
    gSDLAppTitle = apptitle;
    gSDLAppExec = execname;

#ifdef __EMSCRIPTEN__
    // For Emscripten, resources are at /data (virtual filesystem)
    std::string resource_dir = "/data/";
    std::string fonts_dir = "/data/fonts/";
    std::string shaders_dir = "/data/shaders/";
    std::string conf_dir = "/";
#else
    // For native builds, look for resources relative to executable or in standard locations
    std::string resource_dir;
    std::string fonts_dir;
    std::string shaders_dir;
    std::string conf_dir;

    // Try relative to executable first
    if (!exepath.empty()) {
        fs::path exe_path(exepath);
        fs::path exe_dir = exe_path.parent_path();
        fs::path data_path = exe_dir / "data";
        if (fs::is_directory(data_path)) {
            resource_dir = data_path.string() + "/";
            fonts_dir = (data_path / "fonts").string() + "/";
            shaders_dir = (data_path / "shaders").string() + "/";
        }
    }

    // Fallback to current directory
    if (resource_dir.empty()) {
        if (fs::is_directory("data")) {
            resource_dir = "data/";
            fonts_dir = "data/fonts/";
            shaders_dir = "data/shaders/";
        } else if (fs::is_directory("../data")) {
            resource_dir = "../data/";
            fonts_dir = "../data/fonts/";
            shaders_dir = "../data/shaders/";
        } else {
            // Ultimate fallback
            resource_dir = "./";
            fonts_dir = "./fonts/";
            shaders_dir = "./shaders/";
        }
    }

    // Config directory
    const char* home = getenv("HOME");
    if (home) {
        conf_dir = std::string(home) + "/.gource/";
    } else {
        conf_dir = "./";
    }
#endif

    texturemanager.setDir(resource_dir);
    fontmanager.setDir(fonts_dir);
    shadermanager.setDir(shaders_dir);

    gSDLAppResourceDir = resource_dir;
    gSDLAppConfDir = conf_dir;

    fontmanager.init();
}

void SDLAppParseArgs(int argc, char* argv[], int* xres, int* yres, bool* fullscreen, std::vector<std::string>* otherargs) {
    // For web, we typically don't have command line args
    // Configuration comes from JavaScript
}

SDLApp::SDLApp() {
    fps = 0;
    return_code = 0;
    appFinished = false;
    min_delta_msec = 8;
    frame_count = 0;
    fps_updater = 0;
}

void SDLApp::updateFramerate() {
    if (fps_updater > 0) {
        fps = (float)frame_count / (float)fps_updater * 1000.0f;
    } else {
        fps = 0;
    }
    fps_updater = 0;
    frame_count = 0;
}

bool SDLApp::isFinished() {
    return appFinished;
}

int SDLApp::returnCode() {
    return return_code;
}

void SDLApp::stop(int return_code) {
    this->return_code = return_code;
    appFinished = true;
}

bool SDLApp::handleEvent(SDL_Event& event) {
    switch (event.type) {
        case SDL_QUIT:
            quit();
            break;

        case SDL_MOUSEMOTION:
            mouseMove(&event.motion);
            break;

        case SDL_TEXTINPUT:
            textInput(&event.text);
            break;

        case SDL_TEXTEDITING:
            textEdit(&event.edit);
            break;

        case SDL_MOUSEWHEEL:
            mouseWheel(&event.wheel);
            break;

        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                resize(event.window.data1, event.window.data2);
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            mouseClick(&event.button);
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
            keyPress(&event.key);
            break;

        default:
            return false;
    }
    return true;
}

#ifdef __EMSCRIPTEN__
// Global state for Emscripten main loop
static SDLApp* g_app = nullptr;
static Uint32 g_last_msec = 0;
static Uint32 g_total_msec = 0;

static int g_frame_log_count = 0;

static void emscripten_main_loop() {
    if (!g_app || g_app->isFinished()) {
        printf("Main loop: app finished or null\n");
        emscripten_cancel_main_loop();
        return;
    }

    Uint32 msec = SDL_GetTicks();
    Uint32 delta_msec = msec - g_last_msec;
    g_last_msec = msec;

    // Cap delta to prevent physics explosion on first frame or after tab switch
    // Max 100ms (10 FPS minimum)
    if (delta_msec > 100) {
        delta_msec = 100;
    }

    g_total_msec += delta_msec;

    float t = g_total_msec / 1000.0f;
    float dt = delta_msec / 1000.0f;

    g_app->fps_updater += delta_msec;
    if (g_app->fps_updater >= 1000) {
        g_app->updateFramerate();
        printf("FPS: %.1f, t=%.1f\n", g_app->fps, t);
    }

    // Process events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN) {
            printf("KEYDOWN: scancode=%d sym=%d mod=%d\n",
                   event.key.keysym.scancode, event.key.keysym.sym, event.key.keysym.mod);
        } else if (event.type == SDL_KEYUP) {
            printf("KEYUP: scancode=%d sym=%d\n",
                   event.key.keysym.scancode, event.key.keysym.sym);
        } else if (g_frame_log_count < 5) {
            printf("Event type: %d\n", event.type);
        }
        g_app->handleEvent(event);
    }

    g_app->update(t, dt);
    display.update();
    g_app->frame_count++;

    if (g_frame_log_count < 5) {
        printf("Frame %d: t=%.2f dt=%.4f\n", g_app->frame_count, t, dt);
        g_frame_log_count++;
    }
}
#endif

int SDLApp::run() {
    init();

    SDL_StopTextInput();

#ifdef __EMSCRIPTEN__
    g_app = this;
    g_last_msec = SDL_GetTicks();
    g_total_msec = 0;

    // Use 0 for fps to let browser control timing (requestAnimationFrame)
    // Use 0 for simulate_infinite_loop since we may be called from JS callback
    emscripten_set_main_loop(emscripten_main_loop, 0, 0);
#else
    // Native main loop
    Uint32 last_msec = SDL_GetTicks();
    Uint32 total_msec = 0;

    while (!appFinished) {
        Uint32 msec = SDL_GetTicks();
        Uint32 delta_msec = msec - last_msec;
        last_msec = msec;

        // Cap minimum frame time for stability
        if (delta_msec < min_delta_msec) {
            SDL_Delay(min_delta_msec - delta_msec);
            delta_msec = min_delta_msec;
        }

        total_msec += delta_msec;

        float t = total_msec / 1000.0f;
        float dt = delta_msec / 1000.0f;

        fps_updater += delta_msec;
        if (fps_updater >= 1000) {
            updateFramerate();
        }

        // Process events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        update(t, dt);
        display.update();
        frame_count++;
    }
#endif

    return return_code;
}
