// === File: src/main.cpp =======================================================
// AGENT: PURPOSE    — Gource Web entry point for Emscripten
// AGENT: STATUS     — in-progress (2026-01-10)
// =============================================================================

#include "main.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <fstream>
#endif

static bool g_gource_started = false;
static GourceShell* g_gourcesh = nullptr;
static ConfFile* g_conf = nullptr;

// Note: The main loop is handled by SDLApp::run() which sets up
// emscripten_set_main_loop() internally for WebAssembly builds

// C API for JavaScript
extern "C" {

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void gource_reset() {
    printf("gource_reset: cleaning up...\n");

#ifdef __EMSCRIPTEN__
    // Cancel the main loop first
    emscripten_cancel_main_loop();
#endif

    // Clean up existing GourceShell
    if (g_gourcesh) {
        delete g_gourcesh;
        g_gourcesh = nullptr;
        gGourceShell = nullptr;
    }

    g_gource_started = false;
    printf("gource_reset: done\n");
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
int gource_load_log(const char* log_data) {
    if (!log_data || strlen(log_data) == 0) {
        printf("gource_load_log: no data or empty\n");
        return 0;
    }

    printf("Log data received: %zu bytes\n", strlen(log_data));

    // If already running, reset first
    if (g_gource_started) {
        printf("Gource already running, resetting...\n");
        gource_reset();
    }

#ifdef __EMSCRIPTEN__
    // Write log data to Emscripten virtual filesystem
    {
        std::ofstream outfile("/gource.log");
        if (!outfile.is_open()) {
            printf("ERROR: Could not create /gource.log\n");
            return 0;
        }
        outfile << log_data;
        outfile.close();
        printf("Wrote log to /gource.log\n");
    }

    // Set the path to our virtual log file
    gGourceSettings.path = "/gource.log";
    gGourceSettings.default_path = false;

    // Add a gource section to the config with the path
    ConfSection* gource_section = g_conf->addSection("gource");
    gource_section->setEntry("path", "/gource.log");

    printf("Starting Gource visualization...\n");

    try {
        g_gourcesh = gGourceShell = new GourceShell(g_conf, nullptr);
        g_gource_started = true;
        g_gourcesh->run();  // This will set up emscripten_set_main_loop
    } catch (ResourceException& exception) {
        printf("ERROR: failed to load resource '%s'\n", exception.what());
        return 0;
    } catch (SDLAppException& exception) {
        printf("ERROR: %s\n", exception.what());
        return 0;
    }
#endif

    return 1;
}

}

int main(int argc, char* argv[]) {
    printf("Gource Web starting...\n");

    try {
        SDLAppInit("Gource", "gource", "");
        printf("SDLAppInit done\n");
    } catch (std::exception& e) {
        printf("SDLAppInit failed: %s\n", e.what());
        return 1;
    }

    g_conf = new ConfFile();
    printf("ConfFile created\n");

    try {
        // Set default settings for web
        gGourceSettings.setGourceDefaults();
        printf("Defaults set\n");

        // Override some settings for web
        gGourceSettings.log_level = LOG_LEVEL_WARN;

        Logger::getDefault()->setLevel(gGourceSettings.log_level);

        // Import settings - skip for web since we have no config file
        // gGourceSettings.importDisplaySettings(*g_conf);
        // gGourceSettings.importGourceSettings(*g_conf);

    } catch (ConfFileException& exception) {
        printf("ConfFileException: %s\n", exception.what());
        return 1;
    } catch (std::exception& e) {
        printf("Exception during settings: %s\n", e.what());
        return 1;
    }

    // Enable vsync for web
    display.enableVsync(true);

    // Allow resizing
    display.enableResize(true);

    printf("About to init display...\n");
    fflush(stdout);

    try {
        display.init("Gource", gGourceSettings.display_width, gGourceSettings.display_height, false);
        printf("Display initialized\n");
        fflush(stdout);
    } catch (SDLInitException& exception) {
        printf("SDL initialization failed: %s\n", exception.what());
        fflush(stdout);
        return 1;
    } catch (std::exception& e) {
        printf("Display init exception: %s\n", e.what());
        fflush(stdout);
        return 1;
    }

    printf("Gource initialized. Waiting for log data...\n");
    printf("Call gource_load_log() from JavaScript to start visualization.\n");
    fflush(stdout);

#ifdef __EMSCRIPTEN__
    // For Emscripten, we don't start GourceShell immediately
    // Instead we wait for log data to be loaded via JavaScript
    // With EXIT_RUNTIME=0, the runtime stays alive after main() returns
    // allowing JavaScript to call gource_load_log() later
    printf("Emscripten: main() complete, waiting for JavaScript to load log data\n");
    fflush(stdout);
    return 0;
#else
    // Native build - require a log file argument
    GourceShell* gourcesh = nullptr;

    try {
        gourcesh = gGourceShell = new GourceShell(g_conf, nullptr);
        gourcesh->run();
    } catch (ResourceException& exception) {
        char errormsg[1024];
        snprintf(errormsg, 1024, "failed to load resource '%s'", exception.what());
        SDLAppQuit(errormsg);
    } catch (SDLAppException& exception) {
        if (exception.showHelp()) {
            gGourceSettings.help();
        } else {
            SDLAppQuit(exception.what());
        }
    }

    gGourceShell = nullptr;

    if (gourcesh != nullptr) delete gourcesh;

    display.quit();

    return 0;
#endif
}

