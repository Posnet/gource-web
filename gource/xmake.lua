-- === File: xmake.lua =========================================================
-- AGENT: PURPOSE    — Build configuration for Gource WebAssembly
-- AGENT: STATUS     — in-progress (2026-01-09)
-- =============================================================================

set_project("gource-web")
set_version("0.56.0")
set_languages("c++17")

add_rules("mode.debug", "mode.release")

-- Dependencies from source (xmake will fetch and compile)
-- system = false forces building from source instead of using homebrew/system packages
add_requires("libsdl2", {system = false, configs = {shared = false}})
add_requires("libsdl2_image", {system = false, configs = {shared = false}})
add_requires("freetype", {system = false, configs = {shared = false}})
add_requires("pcre2", {system = false, configs = {shared = false}})
add_requires("libpng", {system = false, configs = {shared = false}})
add_requires("glm", {system = false})
add_requires("zlib", {system = false})

-- Common source files
local src_files = {
    "src/main.cpp",
    "src/action.cpp",
    "src/bloom.cpp",
    "src/caption.cpp",
    "src/dirnode.cpp",
    "src/file.cpp",
    "src/gource.cpp",
    "src/gource_shell.cpp",
    "src/gource_settings.cpp",
    "src/key.cpp",
    "src/logmill.cpp",
    "src/pawn.cpp",
    "src/slider.cpp",
    "src/spline.cpp",
    "src/textbox.cpp",
    "src/user.cpp",
    "src/zoomcamera.cpp",
    "src/formats/apache.cpp",
    "src/formats/bzr.cpp",
    "src/formats/commitlog.cpp",
    "src/formats/custom.cpp",
    "src/formats/cvs-exp.cpp",
    "src/formats/cvs2cl.cpp",
    "src/formats/git.cpp",
    "src/formats/gitraw.cpp",
    "src/formats/hg.cpp",
    "src/formats/svn.cpp",
    "src/core/conffile.cpp",
    "src/core/display.cpp",
    "src/core/frustum.cpp",
    "src/core/fxfont.cpp",
    "src/core/logger.cpp",
    "src/core/mousecursor.cpp",
    "src/core/plane.cpp",
    "src/core/png_writer.cpp",
    "src/core/ppm.cpp",
    "src/core/quadtree.cpp",
    "src/core/regex.cpp",
    "src/core/resource.cpp",
    "src/core/sdlapp.cpp",
    "src/core/seeklog.cpp",
    "src/core/settings.cpp",
    "src/core/shader.cpp",
    "src/core/shader_common.cpp",
    "src/core/stringhash.cpp",
    "src/core/texture.cpp",
    "src/core/timezone.cpp",
    "src/core/vbo.cpp",
    "src/core/vectors.cpp",
    "src/core/renderer.cpp",
    "src/core/ui/button.cpp",
    "src/core/ui/checkbox.cpp",
    "src/core/ui/colour.cpp",
    "src/core/ui/console.cpp",
    "src/core/ui/element.cpp",
    "src/core/ui/group.cpp",
    "src/core/ui/image.cpp",
    "src/core/ui/label.cpp",
    "src/core/ui/layout.cpp",
    "src/core/ui/scroll_bar.cpp",
    "src/core/ui/scroll_layout.cpp",
    "src/core/ui/select.cpp",
    "src/core/ui/slider.cpp",
    "src/core/ui/solid_layout.cpp",
    "src/core/ui/subgroup.cpp",
    "src/core/ui/ui.cpp",
    "src/tinyxml/tinystr.cpp",
    "src/tinyxml/tinyxml.cpp",
    "src/tinyxml/tinyxmlerror.cpp",
    "src/tinyxml/tinyxmlparser.cpp"
}

-- Native target (macOS/Linux)
target("gource-native")
    set_kind("binary")

    add_files(src_files)
    add_includedirs("src", "src/core", "src/tinyxml")

    add_packages("libsdl2", "libsdl2_image", "freetype", "pcre2", "libpng", "glm", "zlib")

    add_cxflags("-Wall", "-Wno-sign-compare", "-Wno-reorder", "-Wno-unused-variable")

    add_defines("PCRE2_CODE_UNIT_WIDTH=8")
    add_defines('SDLAPP_RESOURCE_DIR="./data"')

    if is_plat("macosx") then
        add_frameworks("OpenGL", "Cocoa", "IOKit", "CoreVideo", "CoreFoundation", "Carbon", "ForceFeedback", "GameController", "CoreHaptics")
    elseif is_plat("linux") then
        add_syslinks("GL", "dl", "pthread")
    end

    if is_mode("debug") then
        add_defines("DEBUG")
        set_symbols("debug")
    else
        set_optimize("faster")
    end
target_end()

-- Emscripten/WebAssembly target
-- Build with: xmake f -p wasm && xmake gource-web
target("gource-web")
    set_kind("binary")
    set_extension(".js")

    -- Only build when targeting wasm
    on_load(function (target)
        if not is_plat("wasm") then
            target:set("enabled", false)
        end
    end)

    add_files(src_files)
    add_includedirs("src", "src/core", "src/tinyxml")

    -- For Emscripten, use glm (header-only) and pcre2 (built from source)
    -- Other deps come from Emscripten ports via compiler flags
    add_packages("glm", "pcre2")

    -- Emscripten provides SDL2, SDL2_image, FreeType, libpng via ports
    add_cxflags("-sUSE_SDL=2", "-sUSE_SDL_IMAGE=2", "-sUSE_FREETYPE=1", "-sUSE_LIBPNG=1", {force = true})
    add_cxflags("-Wall", "-Wno-sign-compare", "-Wno-reorder", "-Wno-unused-variable")

    add_ldflags("-sUSE_SDL=2", "-sUSE_SDL_IMAGE=2", "-sUSE_FREETYPE=1", "-sUSE_LIBPNG=1", {force = true})
    add_ldflags("-sSDL2_IMAGE_FORMATS=[\"png\",\"jpg\"]", {force = true})
    add_ldflags("-sUSE_WEBGL2=1", "-sMIN_WEBGL_VERSION=2", "-sMAX_WEBGL_VERSION=2", {force = true})
    add_ldflags("-sFULL_ES3=1", {force = true})
    add_ldflags("-sALLOW_MEMORY_GROWTH=1", "-sINITIAL_MEMORY=128MB", "-sSTACK_SIZE=2MB", {force = true})
    add_ldflags("-sEXIT_RUNTIME=0", "-sNO_EXIT_RUNTIME=1", {force = true})
    add_ldflags("-sNO_DISABLE_EXCEPTION_CATCHING", {force = true})
    add_cxflags("-fexceptions", {force = true})
    add_ldflags("-sEXPORTED_RUNTIME_METHODS=[ccall,cwrap,UTF8ToString]", {force = true})
    add_ldflags("-sEXPORTED_FUNCTIONS=[_main,_gource_load_log]", {force = true})
    add_ldflags("-sMODULARIZE=1", "-sEXPORT_NAME=GourceModule", {force = true})
    add_ldflags("--preload-file", "data@/data", {force = true})

    add_defines("PCRE2_CODE_UNIT_WIDTH=8")
    add_defines('SDLAPP_RESOURCE_DIR="/data"')

    if is_mode("debug") then
        add_defines("DEBUG")
        set_symbols("debug")
        add_cxflags("-g", "-gsource-map", {force = true})
        add_ldflags("-g", "-gsource-map", "--source-map-base", "http://localhost:8080/", {force = true})
        add_ldflags("-sASSERTIONS=2", "-sGL_DEBUG=1", {force = true})
    else
        set_optimize("faster")
        add_cxflags("-flto")
        add_ldflags("-flto", "-sASSERTIONS=0", {force = true})
    end
target_end()
