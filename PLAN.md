# Plan — Gource Web (2026-01-09)

## Goals
- Compile Gource for WebAssembly/Emscripten to run in browser
- Provide the same interactive visualizer experience in a web browser
- Accept git log data via API call (no local VCS operations needed)

## Non-Goals
- Support for local git/svn/hg/bzr/cvs operations (log provided via API)
- PPM/video export functionality (browser has other export options)
- File selector UI (log data comes from API)
- Multi-monitor/screen selection support

## Constraints
- Must use WebGL (OpenGL ES 2.0/3.0 subset)
- Cannot use GLEW (WebGL handles extensions differently)
- Cannot use GLU (deprecated, not available in WebGL)
- Cannot use Boost.Filesystem (no local filesystem in browser)
- Fixed-function OpenGL pipeline not available in WebGL
- Must use modern GLSL shaders (WebGL 1.0 = GLSL ES 1.0, WebGL 2.0 = GLSL ES 3.0)

---

## Dependency Audit

### Current Dependencies (from configure.ac)

| Dependency | Version | Web Compatibility | Action Required |
|------------|---------|-------------------|-----------------|
| **SDL2** | - | SUPPORTED via Emscripten port | Use `emcmake` with SDL2 port |
| **SDL2_image** | - | SUPPORTED via Emscripten port | Use with PNG/JPEG support |
| **OpenGL** | - | PARTIAL - WebGL only | Remove fixed-function calls |
| **GLU** | - | NOT SUPPORTED | Replace gluPerspective, gluProject, gluUnProject, gluErrorString |
| **GLEW** | - | NOT NEEDED | Remove entirely (WebGL handles extensions) |
| **FreeType2** | >= 9.0.3 | SUPPORTED via Emscripten port | Use Emscripten port |
| **PCRE2** | - | SUPPORTED via Emscripten port | Use Emscripten port |
| **libpng** | >= 1.2 | SUPPORTED via Emscripten port | Use Emscripten port |
| **Boost.Filesystem** | >= 1.69 | NOT SUPPORTED | Replace with C++17 std::filesystem or custom API |
| **Boost.Algorithm** | - | NOT SUPPORTED | Replace with standard algorithms |
| **Boost.Assign** | - | NOT SUPPORTED | Replace with initializer lists |
| **GLM** | - | SUPPORTED (header-only) | Keep as-is |
| **TinyXML** | bundled | SUPPORTED | Keep bundled version |

### Submodules
- `src/core` - Core library (acaudwell/Core) - requires significant modification

---

## Code Audit - Critical Issues

### 1. Fixed-Function OpenGL (BLOCKING)

The codebase uses deprecated OpenGL 1.x/2.x fixed-function pipeline extensively:

**Files affected:**
- `src/action.cpp` - glBegin/glEnd, glVertex, glColor, glTexCoord
- `src/bloom.cpp` - glVertexPointer, glColorPointer, glTexCoordPointer
- `src/dirnode.cpp` - glBegin/glEnd, glVertex, glColor, glTexCoord
- `src/gource.cpp` - glBegin/glEnd, glVertex, glColor
- `src/gource_shell.cpp` - glBegin/glEnd, glVertex, glColor, glTexCoord
- `src/core/display.cpp` - glMatrixMode, glLoadIdentity, glOrtho, gluPerspective, gluProject, gluUnProject

**Functions to replace:**
- `glBegin()`/`glEnd()` → VBO-based rendering
- `glVertex*()` → vertex buffer attributes
- `glColor*()` → vertex buffer attributes or uniforms
- `glTexCoord*()` → vertex buffer attributes
- `glMatrixMode()`/`glLoadIdentity()`/`glPushMatrix()`/`glPopMatrix()` → manual matrix stack with GLM
- `gluPerspective()` → `glm::perspective()`
- `gluProject()` → `glm::project()`
- `gluUnProject()` → `glm::unProject()`
- `gluErrorString()` → custom error string function
- `ftransform()` in shaders → manual MVP transform

### 2. Shaders (BLOCKING)

Current shaders use legacy GLSL (no version, deprecated built-ins):

**Files:** `data/shaders/*.vert`, `data/shaders/*.frag`

**Issues:**
- No `#version` directive
- Uses `gl_Vertex`, `gl_Color`, `gl_MultiTexCoord0` (deprecated)
- Uses `varying` (WebGL 2 uses `in`/`out`)
- Uses `ftransform()` (deprecated)
- Uses `gl_FrontColor` (deprecated)

**Required changes:**
- Add `#version 100` (WebGL 1) or `#version 300 es` (WebGL 2)
- Replace built-in attributes with explicit attribute declarations
- Pass MVP matrix as uniform
- Use `in`/`out` instead of `varying` for WebGL 2

### 3. GLEW Dependency (BLOCKING)

**File:** `src/core/gl.h`, `src/core/display.cpp`

GLEW is not needed for WebGL. Emscripten provides OpenGL ES headers directly.

**Changes:**
- Replace `#include <GL/glew.h>` with Emscripten GL headers
- Remove `glewInit()` call
- Remove GLEW error checking

### 4. GLU Dependency (BLOCKING)

**File:** `src/core/display.cpp`, `src/core/gl.h`

GLU functions used:
- `gluPerspective()` - use `glm::perspective()`
- `gluProject()` - use `glm::project()`
- `gluUnProject()` - use `glm::unProject()`
- `gluErrorString()` - implement custom function

### 5. Boost.Filesystem (BLOCKING)

**Files:**
- `src/core/resource.cpp` - `boost::filesystem::is_regular_file`, `is_directory`
- `src/core/ui/file_selector.cpp` - extensive filesystem operations

**Changes:**
- Replace with C++17 `std::filesystem` where applicable
- For web: implement virtual filesystem or remove file selector entirely
- Log data will come from API, not local files

### 6. Platform-Specific Code (MEDIUM)

**Windows-specific:**
- `src/core/sdlapp.cpp` - console handling (not needed for web)
- `src/core/display.cpp` - DPI awareness, window filtering

**Mac-specific:**
- CoreFoundation framework linking

**X11-specific:**
- Clipboard handling

**Changes:** Wrap with `#ifdef __EMSCRIPTEN__` or remove

### 7. VCS Format Parsers (LOW - can keep)

**Files:** `src/formats/*.cpp`

These parse log formats (git, svn, hg, bzr, cvs). The custom format parser (`custom.cpp`) is what we need for API input.

**Keep:** `custom.cpp`, `commitlog.cpp`
**Remove or disable:** `git.cpp`, `svn.cpp`, `hg.cpp`, `bzr.cpp`, `cvs2cl.cpp`, `cvs-exp.cpp`, `apache.cpp`, `gitraw.cpp`

### 8. File I/O (MEDIUM)

**Files:**
- `src/core/seeklog.cpp` - file seeking for log files
- `src/core/conffile.cpp` - config file parsing
- `src/core/ppm.cpp` - PPM export (not needed)
- `src/core/png_writer.cpp` - screenshot export

**Changes:**
- Log data will be provided as in-memory string/buffer from JavaScript
- Config can be passed as JSON from JavaScript
- Screenshots can use canvas.toDataURL() instead

### 9. Main Entry Point (MEDIUM)

**File:** `src/main.cpp`

Needs to be restructured for Emscripten:
- Use `emscripten_set_main_loop()` instead of blocking main loop
- Export functions for JavaScript interop
- Accept log data from JavaScript

---

## Interfaces Touched

- `src/core/gl.h` - GL header abstraction
- `src/core/display.cpp` - Display/window management
- `src/core/sdlapp.cpp` - SDL application framework
- `src/core/shader.cpp` - Shader loading/compilation
- `src/main.cpp` - Entry point
- `data/shaders/*` - All shader files

---

## Risks

- **R1:** Fixed-function to modern GL conversion is extensive - mitigation: incremental conversion, start with rendering pipeline
- **R2:** Shader rewrite may introduce visual differences - mitigation: test on native OpenGL ES first
- **R3:** Performance regression in browser - mitigation: profile, use WebGL 2 where possible
- **R4:** FreeType font rendering complexity - mitigation: may need to pre-render fonts or use alternative

---

## Breaking Changes

- **BC1:** Remove all VCS command execution (git, svn, etc.) - log provided via API
- **BC2:** Remove file selector UI - not applicable to web
- **BC3:** Remove PPM export - browser has canvas export
- **BC4:** Remove config file loading from disk - config via JavaScript
- **BC5:** Remove Boost dependency entirely - use C++17 or custom solutions

---

## Recommended Approach

### Phase 1: Build System Setup
- Create CMakeLists.txt for Emscripten
- Configure Emscripten ports (SDL2, SDL2_image, FreeType, PCRE2, libpng)
- Set up WebGL context creation

### Phase 2: Remove/Replace Incompatible Dependencies
- Remove GLEW
- Remove GLU (replace with GLM equivalents)
- Remove Boost.Filesystem
- Remove platform-specific code (Windows console, X11 clipboard)

### Phase 3: Modernize OpenGL Code
- Create abstraction layer for GL calls
- Convert immediate mode rendering to VBO-based
- Implement matrix stack using GLM
- Update all shaders to GLSL ES

### Phase 4: Adapt for Browser Environment
- Create JavaScript API for passing log data
- Use Emscripten main loop
- Implement virtual filesystem for resources (fonts, textures, shaders)
- Export control functions to JavaScript

### Phase 5: Integration
- Create HTML/JavaScript wrapper
- Implement log data fetching from API
- Add browser-specific controls (touch, etc.)

---

## Bench Targets
- Initial load time: < 3 seconds for 10k commit log
- Frame rate: 60 FPS during normal playback
- Memory usage: < 512MB for typical repository

---

## Next Actions

- PX-1: Create CMakeLists.txt for Emscripten build — files: `CMakeLists.txt` — exit: `emcmake cmake` succeeds
- PX-2: Create GL abstraction header for WebGL — files: `src/core/gl.h`, `src/core/gl_compat.h` — exit: compiles without GLEW
- PX-3: Replace GLU functions with GLM — files: `src/core/display.cpp` — exit: no GLU references
- PX-4: Remove Boost.Filesystem usage — files: `src/core/resource.cpp`, `src/core/ui/file_selector.cpp` — exit: no boost::filesystem references
- PX-5: Convert shaders to GLSL ES — files: `data/shaders/*` — exit: shaders compile in WebGL context
- PX-6: Convert immediate mode rendering to VBOs — files: multiple src/*.cpp — exit: no glBegin/glEnd calls

---

## Evidence

**Native Build (2026-01-10):**
```
xmake build gource-native
[100%]: build ok, spent 0.74s
```

**WebAssembly Build (2026-01-10):**
```
source ~/emsdk/emsdk_env.sh && xmake f -p wasm -y && xmake build gource-web
[100%]: build ok, spent 33.3s

Build artifacts:
- gource-web.js    (228 KB) - JavaScript glue code
- gource-web.wasm  (2.0 MB) - WebAssembly binary
- gource-web.data  (1.2 MB) - Preloaded assets (shaders, fonts, textures)
```

**Files Created/Modified:**
- `xmake.lua` - Build system for Emscripten with all source files
- `src/core/gl.h` - WebGL-only GL headers
- `src/core/renderer.h/.cpp` - Modern VBO-based renderer
- `src/core/display.h/.cpp` - WebGL display management, HiDPI support
- `src/core/sdlapp.h/.cpp` - Emscripten main loop + native support
- `src/core/resource.cpp` - Removed boost::filesystem
- `src/core/shader.cpp` - Removed boost::format, geometry/compute shaders
- `src/core/shader_common.cpp` - Removed boost::format with snprintf helper
- `src/core/logger.cpp` - Removed boost::assign, using C++11 initializer list
- `src/core/fxfont.cpp` - Converted to renderer (GL_TRIANGLES)
- `src/core/quadtree.cpp` - Removed display lists (not in WebGL)
- `src/core/vbo.cpp` - Rewritten with VAO and modern vertex attributes
- `src/core/texture.cpp` - GL_RED instead of GL_LUMINANCE, glGenerateMipmap
- `src/core/mousecursor.cpp` - Converted to renderer
- `src/spline.cpp` - Converted to renderer
- `src/zoomcamera.cpp` - glm::lookAt instead of gluLookAt
- `src/formats/hg.cpp`, `svn.cpp`, `bzr.cpp` - Removed boost::format
- `src/core/ui/*.cpp` - GL_CLAMP_TO_EDGE instead of GL_CLAMP
- `src/main.cpp` - Emscripten entry point with JS API
- `data/shaders/*.vert/*.frag` - GLSL ES 3.0 shaders
- `web/index.html`, `web/gource.css`, `web/gource.js` - Web interface

---

## Log

- [2026-01-09 19:10] Initial audit completed. Gource cloned with Core submodule. Identified 9 major compatibility issues for web compilation.
- [2026-01-09 19:XX] Created xmake.lua build system for Emscripten with SDL2, FreeType, PCRE2 ports
- [2026-01-09 19:XX] Created WebGL-only gl.h header (removed GLEW, GLU)
- [2026-01-09 19:XX] Created modern Renderer class with VBO-based rendering, matrix stack
- [2026-01-09 19:XX] Rewrote all shaders for GLSL ES 3.0 (version 300 es)
- [2026-01-09 19:XX] Rewrote display.cpp and sdlapp.cpp for Emscripten main loop
- [2026-01-09 19:XX] Removed all boost dependencies (filesystem, format, assign)
- [2026-01-09 19:XX] Created web interface (HTML, CSS, JS) with log loading API
- [2026-01-10] Completed native macOS build with xmake:
  - Fixed fxfont.cpp: converted to renderer (GL_TRIANGLES instead of GL_QUADS)
  - Fixed quadtree.cpp: removed display lists (glGenLists, glNewList, etc.)
  - Added missing source files to xmake.lua (formats/*.cpp, png_writer.cpp, ppm.cpp)
  - Removed boost::format from hg.cpp, svn.cpp, bzr.cpp with string concatenation
  - Native build successful: `xmake build gource-native` completes
- [2026-01-10] Completed WebAssembly build:
  - Installed Emscripten SDK 4.0.22
  - Added GLdouble typedef for OpenGL ES compatibility
  - Made SDLApp::updateFramerate() and handleEvent() public for Emscripten callback
  - Fixed texture.cpp: convert BGRA/BGR surfaces to RGBA/RGB (WebGL doesn't support BGRA)
  - WebAssembly build successful: `xmake build gource-web` produces .js, .wasm, .data
- [2026-01-10] WebGL debugging and fixes:
  - Fixed shader version string: preprocessor was dropping `es` suffix from `#version 300 es`
  - Added `precision highp float;` right after version for GLSL ES shaders
  - Converted TGA textures (bloom.tga, bloom_alpha.tga) to PNG for WebGL compatibility
  - Added GL_TEXTURE_2D compatibility wrapper macros (glEnable/glDisable of GL_TEXTURE_2D invalid in ES)
  - Fixed canvas keyboard focus for web input
  - Fixed vsync timing: skipped SDL_GL_SetSwapInterval for Emscripten (browser controls via rAF)
  - Generated proper demo.log with 500 entries and 181 unique timestamps
  - Added help modal with keyboard controls (? button)
  - **Fixed logmill threading**: SDL_CreateThread doesn't work well in Emscripten, now runs synchronously
- [2026-01-10] Font/text rendering fixes:
  - **Fixed font texture format**: Changed `GL_ALPHA` to `GL_RED` in fxfont.cpp (WebGL 2/ES 3.0 doesn't support GL_ALPHA)
  - **Fixed text shader**: Changed texture sampling from `.a` to `.r` to match GL_RED format
  - **Fixed quadbuf::draw()**: Rewrote to use index buffer for quad-to-triangle conversion instead of renderer().begin()/end() which was overriding the text shader
  - **Fixed text shader uniform names**: Changed "tex" to "u_texture", "shadow_strength" to "u_shadow_strength", "texel_size" to "u_texel_size"
  - **Added MVP matrix for text shader**: Set u_mvp uniform before drawing font buffer
  - **Fixed font texture swizzle**: Added texture swizzle so GL_RED texture behaves like alpha (R,G,B→1, A→red channel)
- [2026-01-10] Physics and rendering fixes:
  - **Fixed delta time cap**: Added 100ms cap in Emscripten main loop to prevent physics explosion on first frame or tab switch
  - **Added physics safeguards in dirnode.cpp**: Acceleration clamping (max 1000), position clamping (max 50000), NaN/Inf checks
  - **Fixed duplicate rendering (shadows appearing as copies)**:
    - Shadow draws were using basic_shader instead of shadow_shader
    - Added `use_own_shader` parameter to quadbuf::draw() - pass false when external shader is bound
    - Fixed shadow_shader uniform names: "tex"→"u_texture", "shadow_strength"→"u_shadow_strength"
    - Added MVP matrix setup for shadow_shader, bloom_shader before drawing
    - Fixed edge, action, bloom draws to properly set MVP and shader flags

## Remaining Work

Converted to use Renderer class:
- `src/action.cpp` - DONE
- `src/bloom.cpp` - DONE (rewrote with VAO/VBO)
- `src/dirnode.cpp` - DONE (replaced gluProject, matrix stack, immediate mode)
- `src/gource.cpp` - DONE (replaced GLEW checks, matrix ops, immediate mode)
- `src/gource_shell.cpp` - DONE
- `src/pawn.cpp` - DONE
- `src/user.cpp` - DONE (replaced gluProject)
- `src/file.cpp` - DONE (replaced gluProject, removed glLoadName)
- `src/slider.cpp` - DONE
- `src/textbox.cpp` - DONE
- `src/key.cpp` - DONE

Still need conversion (core/ui files with deprecated GL):
- `src/core/ui/solid_layout.cpp`
- `src/core/ui/slider.cpp`
- `src/core/ui/select.cpp`
- `src/core/ui/scroll_layout.cpp`
- `src/core/ui/scroll_bar.cpp`
- `src/core/ui/layout.cpp`
- `src/core/ui/label.cpp`
- `src/core/ui/image.cpp`
- `src/core/ui/colour.cpp`
- `src/core/ui/checkbox.cpp`

## Build Requirements

- **Emscripten SDK required for WebAssembly build**: `emsdk install latest && emsdk activate latest`
- Native macOS build available for testing (requires SDL2, glm, freetype, pcre2 via homebrew)
