// === File: src/core/renderer.cpp =============================================
// AGENT: PURPOSE    — Modern GL renderer implementation for WebGL
// AGENT: STATUS     — in-progress (2026-01-09)
// =============================================================================

#include "renderer.h"
#include "logger.h"
#include <algorithm>

Renderer& Renderer::instance() {
    static Renderer inst;
    return inst;
}

void Renderer::init() {
    if (initialized_) return;

    // Create VAO and VBO for basic rendering
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    // Color (location 1)
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    // Texcoord (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // Create VAO and VBO for bloom rendering (different vertex format)
    glGenVertexArrays(1, &bloom_vao_);
    glGenBuffers(1, &bloom_vbo_);

    glBindVertexArray(bloom_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, bloom_vbo_);

    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BloomVertex), (void*)offsetof(BloomVertex, position));
    glEnableVertexAttribArray(0);

    // Color (location 1)
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(BloomVertex), (void*)offsetof(BloomVertex, color));
    glEnableVertexAttribArray(1);

    // Texcoord (location 2) - vec4 for bloom
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(BloomVertex), (void*)offsetof(BloomVertex, texcoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // Load shaders
    basic_shader_ = shadermanager.grab("basic");
    text_shader_ = shadermanager.grab("text");
    bloom_shader_ = shadermanager.grab("bloom");
    shadow_shader_ = shadermanager.grab("shadow");

    initialized_ = true;
    infoLog("Renderer initialized");
}

void Renderer::shutdown() {
    if (!initialized_) return;

    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (bloom_vbo_) glDeleteBuffers(1, &bloom_vbo_);
    if (bloom_vao_) glDeleteVertexArrays(1, &bloom_vao_);

    vbo_ = vao_ = bloom_vbo_ = bloom_vao_ = 0;
    initialized_ = false;
}

void Renderer::setProjection(const glm::mat4& proj) {
    projection_ = proj;
}

void Renderer::setModelView(const glm::mat4& mv) {
    modelview_ = mv;
}

void Renderer::pushModelView() {
    mv_stack_.push(modelview_);
}

void Renderer::popModelView() {
    if (!mv_stack_.empty()) {
        modelview_ = mv_stack_.top();
        mv_stack_.pop();
    }
}

void Renderer::translateMV(float x, float y, float z) {
    modelview_ = glm::translate(modelview_, glm::vec3(x, y, z));
}

void Renderer::rotateMV(float angle, float x, float y, float z) {
    modelview_ = glm::rotate(modelview_, glm::radians(angle), glm::vec3(x, y, z));
}

void Renderer::scaleMV(float x, float y, float z) {
    modelview_ = glm::scale(modelview_, glm::vec3(x, y, z));
}

void Renderer::mode2D(int width, int height) {
    projection_ = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    modelview_ = glm::mat4(1.0f);
}

void Renderer::mode3D(float fov, float aspect, float znear, float zfar) {
    projection_ = glm::perspective(glm::radians(fov), aspect, znear, zfar);
    modelview_ = glm::mat4(1.0f);
}

void Renderer::push2D(int width, int height) {
    proj_stack_.push(projection_);
    mv_stack_.push(modelview_);
    mode2D(width, height);
}

void Renderer::pop2D() {
    if (!proj_stack_.empty()) {
        projection_ = proj_stack_.top();
        proj_stack_.pop();
    }
    if (!mv_stack_.empty()) {
        modelview_ = mv_stack_.top();
        mv_stack_.pop();
    }
}

void Renderer::begin(GLenum mode) {
    current_mode_ = mode;
    vertices_.clear();
}

void Renderer::end() {
    if (vertices_.empty()) return;

    // Convert quads to triangles (WebGL doesn't support GL_QUADS)
    std::vector<Vertex> draw_verts;
    GLenum draw_mode = current_mode_;

    if (current_mode_ == GL_QUADS) {
        draw_verts = convertQuadsToTriangles(vertices_);
        draw_mode = GL_TRIANGLES;
    } else {
        draw_verts = vertices_;
    }

    // Upload and draw
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, draw_verts.size() * sizeof(Vertex), draw_verts.data(), GL_DYNAMIC_DRAW);

    if (basic_shader_) {
        basic_shader_->bind();

        GLint mvp_loc = basic_shader_->getUniformLocation("u_mvp");
        if (mvp_loc >= 0) {
            glm::mat4 mvp = getMVP();
            glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(mvp));
        }

        GLint use_tex_loc = basic_shader_->getUniformLocation("u_use_texture");
        if (use_tex_loc >= 0) {
            glUniform1i(use_tex_loc, current_texture_ != 0 ? 1 : 0);
        }

        if (current_texture_) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, current_texture_);
            GLint tex_loc = basic_shader_->getUniformLocation("u_texture");
            if (tex_loc >= 0) glUniform1i(tex_loc, 0);
        }

        glDrawArrays(draw_mode, 0, draw_verts.size());
        basic_shader_->unbind();
    }

    glBindVertexArray(0);
    vertices_.clear();
}

void Renderer::vertex(float x, float y, float z) {
    Vertex v;
    v.position = glm::vec3(x, y, z);
    v.color = current_color_;
    v.texcoord = current_texcoord_;
    vertices_.push_back(v);
}

void Renderer::color(float r, float g, float b, float a) {
    current_color_ = glm::vec4(r, g, b, a);
}

void Renderer::texcoord(float s, float t) {
    current_texcoord_ = glm::vec2(s, t);
}

void Renderer::bindTexture(GLuint tex) {
    current_texture_ = tex;
}

void Renderer::bindTexture(TextureResource* tex) {
    current_texture_ = tex ? tex->textureid : 0;
}

void Renderer::unbindTexture() {
    current_texture_ = 0;
}

void Renderer::drawWithShader(Shader* shader) {
    if (vertices_.empty() || !shader) return;

    std::vector<Vertex> draw_verts;
    GLenum draw_mode = current_mode_;

    if (current_mode_ == GL_QUADS) {
        draw_verts = convertQuadsToTriangles(vertices_);
        draw_mode = GL_TRIANGLES;
    } else {
        draw_verts = vertices_;
    }

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, draw_verts.size() * sizeof(Vertex), draw_verts.data(), GL_DYNAMIC_DRAW);

    shader->bind();

    GLint mvp_loc = shader->getUniformLocation("u_mvp");
    if (mvp_loc >= 0) {
        glm::mat4 mvp = getMVP();
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(mvp));
    }

    if (current_texture_) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, current_texture_);
        GLint tex_loc = shader->getUniformLocation("u_texture");
        if (tex_loc >= 0) glUniform1i(tex_loc, 0);
    }

    glDrawArrays(draw_mode, 0, draw_verts.size());

    shader->unbind();
    glBindVertexArray(0);
    vertices_.clear();
}

void Renderer::drawQuad(float x, float y, float w, float h, const glm::vec4& col) {
    begin(GL_QUADS);
    color(col);
    texcoord(0, 0); vertex(x, y);
    texcoord(1, 0); vertex(x + w, y);
    texcoord(1, 1); vertex(x + w, y + h);
    texcoord(0, 1); vertex(x, y + h);
    end();
}

void Renderer::drawQuadTextured(float x, float y, float w, float h, const glm::vec4& col, GLuint tex) {
    bindTexture(tex);
    begin(GL_QUADS);
    color(col);
    texcoord(0, 0); vertex(x, y);
    texcoord(1, 0); vertex(x + w, y);
    texcoord(1, 1); vertex(x + w, y + h);
    texcoord(0, 1); vertex(x, y + h);
    end();
    unbindTexture();
}

void Renderer::drawVertices(GLenum mode, const std::vector<Vertex>& vertices) {
    if (vertices.empty()) return;

    std::vector<Vertex> draw_verts;
    GLenum draw_mode = mode;

    if (mode == GL_QUADS) {
        draw_verts = convertQuadsToTriangles(vertices);
        draw_mode = GL_TRIANGLES;
    } else {
        draw_verts = vertices;
    }

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, draw_verts.size() * sizeof(Vertex), draw_verts.data(), GL_DYNAMIC_DRAW);

    if (basic_shader_) {
        basic_shader_->bind();

        GLint mvp_loc = basic_shader_->getUniformLocation("u_mvp");
        if (mvp_loc >= 0) {
            glm::mat4 mvp = getMVP();
            glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(mvp));
        }

        GLint use_tex_loc = basic_shader_->getUniformLocation("u_use_texture");
        if (use_tex_loc >= 0) {
            glUniform1i(use_tex_loc, current_texture_ != 0 ? 1 : 0);
        }

        if (current_texture_) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, current_texture_);
            GLint tex_loc = basic_shader_->getUniformLocation("u_texture");
            if (tex_loc >= 0) glUniform1i(tex_loc, 0);
        }

        glDrawArrays(draw_mode, 0, draw_verts.size());
        basic_shader_->unbind();
    }

    glBindVertexArray(0);
}

void Renderer::drawBloom(const std::vector<BloomVertex>& vertices) {
    if (vertices.empty() || !bloom_shader_) return;

    // Convert quads to triangles
    std::vector<BloomVertex> draw_verts;
    for (size_t i = 0; i + 3 < vertices.size(); i += 4) {
        draw_verts.push_back(vertices[i]);
        draw_verts.push_back(vertices[i + 1]);
        draw_verts.push_back(vertices[i + 2]);
        draw_verts.push_back(vertices[i]);
        draw_verts.push_back(vertices[i + 2]);
        draw_verts.push_back(vertices[i + 3]);
    }

    glBindVertexArray(bloom_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, bloom_vbo_);
    glBufferData(GL_ARRAY_BUFFER, draw_verts.size() * sizeof(BloomVertex), draw_verts.data(), GL_DYNAMIC_DRAW);

    bloom_shader_->bind();

    GLint mvp_loc = bloom_shader_->getUniformLocation("u_mvp");
    if (mvp_loc >= 0) {
        glm::mat4 mvp = getMVP();
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(mvp));
    }

    glDrawArrays(GL_TRIANGLES, 0, draw_verts.size());
    bloom_shader_->unbind();

    glBindVertexArray(0);
}

std::vector<Vertex> Renderer::convertQuadsToTriangles(const std::vector<Vertex>& quads) {
    std::vector<Vertex> triangles;
    triangles.reserve((quads.size() / 4) * 6);

    for (size_t i = 0; i + 3 < quads.size(); i += 4) {
        // First triangle: 0, 1, 2
        triangles.push_back(quads[i]);
        triangles.push_back(quads[i + 1]);
        triangles.push_back(quads[i + 2]);
        // Second triangle: 0, 2, 3
        triangles.push_back(quads[i]);
        triangles.push_back(quads[i + 2]);
        triangles.push_back(quads[i + 3]);
    }

    return triangles;
}

// GL_QUADS constant for compatibility
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif
