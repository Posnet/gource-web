// === File: src/core/renderer.h ===============================================
// AGENT: PURPOSE    — Modern GL renderer for WebGL, replaces immediate mode
// AGENT: STATUS     — in-progress (2026-01-09)
// =============================================================================

#ifndef CORE_RENDERER_H
#define CORE_RENDERER_H

#include "gl.h"
#include "shader.h"
#include "texture.h"
#include <vector>
#include <stack>

struct Vertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texcoord;
};

struct BloomVertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec4 texcoord;  // x = radius, yzw = center
};

class Renderer {
public:
    static Renderer& instance();

    void init();
    void shutdown();

    // Matrix stack
    void setProjection(const glm::mat4& proj);
    void setModelView(const glm::mat4& mv);
    void pushModelView();
    void popModelView();
    void translateMV(float x, float y, float z);
    void rotateMV(float angle, float x, float y, float z);
    void scaleMV(float x, float y, float z);

    const glm::mat4& getProjection() const { return projection_; }
    const glm::mat4& getModelView() const { return modelview_; }
    glm::mat4 getMVP() const { return projection_ * modelview_; }

    // 2D/3D mode helpers
    void mode2D(int width, int height);
    void mode3D(float fov, float aspect, float znear, float zfar);
    void push2D(int width, int height);
    void pop2D();

    // Immediate-style batched rendering
    void begin(GLenum mode);
    void end();
    void vertex(float x, float y, float z = 0.0f);
    void vertex(const glm::vec2& v) { vertex(v.x, v.y, 0.0f); }
    void vertex(const glm::vec3& v) { vertex(v.x, v.y, v.z); }
    void color(float r, float g, float b, float a = 1.0f);
    void color(const glm::vec3& c) { color(c.x, c.y, c.z, 1.0f); }
    void color(const glm::vec4& c) { color(c.x, c.y, c.z, c.w); }
    void texcoord(float s, float t);
    void texcoord(const glm::vec2& tc) { texcoord(tc.x, tc.y); }

    // Bind texture for next draw
    void bindTexture(GLuint tex);
    void bindTexture(TextureResource* tex);
    void unbindTexture();

    // Draw batched vertices with specific shader
    void drawWithShader(Shader* shader);

    // Convenience drawing functions
    void drawQuad(float x, float y, float w, float h, const glm::vec4& color);
    void drawQuadTextured(float x, float y, float w, float h, const glm::vec4& color, GLuint tex);

    // Direct buffer access for custom rendering
    void drawVertices(GLenum mode, const std::vector<Vertex>& vertices);
    void drawBloom(const std::vector<BloomVertex>& vertices);

    // Current color state
    void setCurrentColor(const glm::vec4& c) { current_color_ = c; }
    glm::vec4 getCurrentColor() const { return current_color_; }

    // Shader access
    Shader* getBasicShader() { return basic_shader_; }
    Shader* getTextShader() { return text_shader_; }
    Shader* getBloomShader() { return bloom_shader_; }

private:
    Renderer() = default;
    ~Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void flush();
    void setupVAO();
    std::vector<Vertex> convertQuadsToTriangles(const std::vector<Vertex>& quads);

    bool initialized_ = false;

    // Matrix stacks
    glm::mat4 projection_{1.0f};
    glm::mat4 modelview_{1.0f};
    std::stack<glm::mat4> mv_stack_;
    std::stack<glm::mat4> proj_stack_;

    // Batching state
    GLenum current_mode_ = 0;
    std::vector<Vertex> vertices_;
    Vertex current_vertex_;
    glm::vec4 current_color_{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec2 current_texcoord_{0.0f, 0.0f};
    GLuint current_texture_ = 0;

    // GL objects
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint bloom_vao_ = 0;
    GLuint bloom_vbo_ = 0;

    // Shaders
    Shader* basic_shader_ = nullptr;
    Shader* text_shader_ = nullptr;
    Shader* bloom_shader_ = nullptr;
    Shader* shadow_shader_ = nullptr;
};

// Global accessor
inline Renderer& renderer() { return Renderer::instance(); }

#endif
