# Gource Web

A WebAssembly port of [Gource](https://github.com/acaudwell/Gource), the software version control visualization tool. Run Gource directly in your web browser without any installation.

## Live Demo

Visit the live demo at: https://gource.denialof.services/

## Features

- Full Gource visualization running in the browser
- Load git logs directly from your local machine
- **GitHub Integration**: Clone and visualize any GitHub repository directly in the browser
- In-browser git clone using [isomorphic-git](https://isomorphic-git.org/) - no API rate limits
- WebGL 2.0 accelerated rendering
- No server-side processing - everything runs client-side

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

## Deployment

The live demo is deployed using [Cloudflare Workers](https://workers.cloudflare.com/) with static assets. The worker handles GitHub OAuth authentication and serves the static files.

### Self-Hosting

To deploy your own instance:

#### 1. Create a GitHub App

1. Go to GitHub Settings → Developer settings → GitHub Apps → New GitHub App
2. Set the following:
   - **App name**: Your app name
   - **Homepage URL**: Your deployment URL
   - **Callback URL**: `https://your-domain.com/api/auth/github/callback`
   - **Webhook**: Uncheck "Active" (not needed)
   - **Permissions**: Repository contents (read-only)
3. Note your **Client ID** and generate a **Client Secret**

#### 2. Configure Wrangler

Create `worker/wrangler.toml`:

```toml
name = "gource-web"
main = "src/index.js"
compatibility_date = "2024-01-01"

routes = [
  { pattern = "your-domain.com", custom_domain = true }
]

[assets]
directory = "../docs"

[vars]
GITHUB_CLIENT_ID = "your-client-id"
```

Set your client secret as a Wrangler secret (not in the config file):

```bash
cd worker
wrangler secret put GITHUB_CLIENT_SECRET
```

#### 3. Deploy

```bash
cd worker
wrangler deploy
```

### Worker Architecture

The Cloudflare Worker (`worker/src/index.js`) handles:

- **Static assets**: Serves files from the `docs/` directory
- **OAuth flow**:
  - `GET /api/auth/github` - Initiates GitHub OAuth
  - `GET /api/auth/github/callback` - Exchanges code for token
- **API proxy**: Proxies GitHub API requests to add authentication and handle CORS

All git operations (cloning, reading commits) happen client-side using isomorphic-git with a CORS proxy.

## Project Structure

```
gource-web/
├── docs/              # Deployed static assets
│   ├── index.html
│   ├── gource.js
│   ├── gource.css
│   ├── gource-web.*   # Built WebAssembly files
│   └── vendor/        # Vendored JS dependencies
├── gource/
│   ├── src/           # Main Gource source code
│   │   ├── core/      # Core library (display, fonts, shaders, etc.)
│   │   ├── formats/   # Log format parsers (git, svn, hg, etc.)
│   │   └── tinyxml/   # XML parsing library
│   ├── data/          # Resources (fonts, shaders, textures)
│   │   └── shaders/   # GLSL ES 3.00 shaders
│   ├── web/           # Development web files (synced to docs/)
│   └── xmake.lua      # Build configuration
├── worker/
│   ├── src/index.js   # Cloudflare Worker source
│   └── wrangler.toml  # Wrangler deployment config
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
