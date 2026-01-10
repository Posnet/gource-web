# Gource Web

A WebAssembly port of [Gource](https://github.com/acaudwell/Gource), the software version control visualization tool. Run Gource directly in your web browser without any installation.

## Live Demo

Visit the live demo at: https://posnet.github.io/gource-web/

## Features

- Full Gource visualization running in the browser
- Load git logs directly from your local machine
- No server-side processing - everything runs client-side
- WebGL 2.0 accelerated rendering

## Usage

1. Generate a git log from your repository:
   ```bash
   git log --pretty=format:"%at|%an|%s" --name-status --no-merges > project.log
   ```

2. Or use the standard Gource custom log format:
   ```bash
   gource --output-custom-log project.log /path/to/repo
   ```

3. Open the web application and select your log file

## Building from Source

### Prerequisites

- [xmake](https://xmake.io/) build system
- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) for WebAssembly builds

### WebAssembly Build

```bash
cd gource

# Setup Emscripten environment
source /path/to/emsdk/emsdk_env.sh

# Configure for WebAssembly
xmake f -p wasm

# Build
xmake build gource-web

# Output files will be in build/wasm/wasm32/*/
# Copy to web directory
cp build/wasm/wasm32/*/gource-web.* web/
```

### Native Build (macOS/Linux)

```bash
cd gource

# Configure for native platform
xmake f -p macosx  # or -p linux

# Build
xmake build gource-native
```

### Running Locally

```bash
cd gource/web

# Start a local web server (Python 3)
python3 -m http.server 8080

# Open http://localhost:8080 in your browser
```

## Dependencies

Dependencies are managed automatically by xmake:

**For WebAssembly builds** (via Emscripten ports):
- SDL2
- SDL2_image
- FreeType
- libpng
- zlib

**For native builds** (via xmake packages):
- SDL2
- SDL2_image
- FreeType
- PCRE2
- libpng
- zlib
- GLM

**Vendored dependencies** (included in source):
- TinyXML
- Core library (from [acaudwell/Core](https://github.com/acaudwell/Core))

## Project Structure

```
gource-web/
├── gource/
│   ├── src/           # Main Gource source code
│   │   ├── core/      # Core library (display, fonts, shaders, etc.)
│   │   ├── formats/   # Log format parsers (git, svn, hg, etc.)
│   │   └── tinyxml/   # XML parsing library
│   ├── data/          # Resources (fonts, shaders, textures)
│   │   └── shaders/   # GLSL ES 3.00 shaders
│   ├── web/           # Web frontend
│   │   ├── index.html
│   │   ├── gource.js
│   │   ├── gource.css
│   │   └── gource-web.*  # Built WebAssembly files
│   └── xmake.lua      # Build configuration
└── README.md
```

## Technical Notes

### WebGL 2.0 / OpenGL ES 3.0

This port uses WebGL 2.0 which requires:
- All shaders use GLSL ES 3.00 (`#version 300 es`)
- `GL_RED` format for single-channel textures (GL_ALPHA not supported)
- Modern attribute/varying syntax (`in`/`out` instead of `attribute`/`varying`)

### Browser Compatibility

Requires a browser with WebGL 2.0 support:
- Chrome 56+
- Firefox 51+
- Safari 15+
- Edge 79+

## Credits

- Original Gource by [Andrew Caudwell](https://github.com/acaudwell/Gource)
- Core library by [Andrew Caudwell](https://github.com/acaudwell/Core)
- WebAssembly port by this project

## License

Gource is licensed under the GNU General Public License version 3. See [gource/COPYING](gource/COPYING) for details.
