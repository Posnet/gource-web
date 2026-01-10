/*
    Copyright (c) 2011 Andrew Caudwell (acaudwell@gmail.com)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. The name of the author may not be used to endorse or promote products
       derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// === File: src/core/vbo.cpp ===================================================
// AGENT: PURPOSE    — VBO/VAO-based quad buffer for modern OpenGL/WebGL
// AGENT: STATUS     — converted for WebGL (2026-01-09)
// =============================================================================

#include "vbo.h"
#include "renderer.h"

//quadbuf

quadbuf::quadbuf(int vertex_capacity) : vertex_capacity(vertex_capacity), vao(0) {
    vertex_count = 0;

    data = vertex_capacity > 0 ? new quadbuf_vertex[vertex_capacity] : 0;
}

quadbuf::~quadbuf() {
    if(data!=0) delete[] data;
    if(vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }
}

void quadbuf::unload() {
    buf.unload();
    if(vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
}

void quadbuf::resize(int new_size) {

    quadbuf_vertex* _data = data;

    data = new quadbuf_vertex[new_size];

    for(int i=0;i<vertex_capacity;i++) {
        data[i] = _data[i];
    }

    vertex_capacity = new_size;

    if(_data != 0) delete[] _data;
}

void quadbuf::reset() {
    textures.resize(0);
    vertex_count = 0;
}

size_t quadbuf::vertices() {
    return vertex_count;
}

size_t quadbuf::capacity() {
    return vertex_capacity;
}

size_t quadbuf::texture_changes() {
    return textures.size();
}

vec4 quadbuf_default_texcoord(0.0f, 0.0f, 1.0f, 1.0f);

void quadbuf::add(GLuint textureid, const vec2& pos, const vec2& dims, const vec4& colour) {
    add(textureid, pos, dims, colour, quadbuf_default_texcoord);
}

void quadbuf::add(GLuint textureid, const vec2& pos, const vec2& dims, const vec4& colour, const vec4& texcoord) {
    //debugLog("%d: %.2f, %.2f, %.2f, %.2f\n", i, pos.x, pos.y, dims.x, dims.y);

    quadbuf_vertex v1(pos,                       colour, vec2(texcoord.x, texcoord.y));
    quadbuf_vertex v2(pos + vec2(dims.x, 0.0f), colour, vec2(texcoord.z, texcoord.y));
    quadbuf_vertex v3(pos + dims,                colour, vec2(texcoord.z, texcoord.w));
    quadbuf_vertex v4(pos + vec2(0.0f, dims.y), colour, vec2(texcoord.x, texcoord.w));

    int i = vertex_count;

    vertex_count += 4;

    if(vertex_count > vertex_capacity) {
        resize(vertex_count*2);
    }

    data[i]   = v1;
    data[i+1] = v2;
    data[i+2] = v3;
    data[i+3] = v4;

    if(textureid>0 && (textures.empty() || textures.back().textureid != textureid)) {
        textures.push_back(quadbuf_tex(i, textureid));
    }
}

void quadbuf::add(GLuint textureid, const quadbuf_vertex& v1, const quadbuf_vertex& v2, const quadbuf_vertex& v3, const quadbuf_vertex& v4) {

    int i = vertex_count;

    vertex_count += 4;

    if(vertex_count > vertex_capacity) {
        resize(vertex_count*2);
    }

    data[i]   = v1;
    data[i+1] = v2;
    data[i+2] = v3;
    data[i+3] = v4;

    if(textureid>0 && (textures.empty() || textures.back().textureid != textureid)) {
        textures.push_back(quadbuf_tex(i, textureid));
    }
}

void quadbuf::initVAO() {
    if(vao == 0) {
        glGenVertexArrays(1, &vao);
    }

    glBindVertexArray(vao);
    buf.bind();

    // Set up vertex attributes
    // Attribute 0: position (vec2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(quadbuf_vertex), (GLvoid*)0);

    // Attribute 1: color (vec4)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(quadbuf_vertex), (GLvoid*)(2 * sizeof(float)));

    // Attribute 2: texcoord (vec2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(quadbuf_vertex), (GLvoid*)(6 * sizeof(float)));

    glBindVertexArray(0);
    buf.unbind();
}

void quadbuf::update() {
    if(vertex_count==0) return;

    //recreate buffer if less than the vertex_count
    buf.buffer( vertex_count, sizeof(quadbuf_vertex), vertex_capacity, &(data[0].pos.x), GL_DYNAMIC_DRAW );

    // Initialize or update VAO
    initVAO();
}

void quadbuf::draw(bool use_own_shader) {
    if(vertex_count==0) return;

    // Build index buffer for quad-to-triangle conversion if needed
    // Each quad (4 vertices) becomes 2 triangles (6 indices): 0,1,2 and 0,2,3
    int num_quads = vertex_count / 4;
    int num_indices = num_quads * 6;

    std::vector<GLushort> indices(num_indices);
    for(int q = 0; q < num_quads; q++) {
        int base_vertex = q * 4;
        int base_index = q * 6;
        // Triangle 1: v0, v1, v2
        indices[base_index + 0] = base_vertex + 0;
        indices[base_index + 1] = base_vertex + 1;
        indices[base_index + 2] = base_vertex + 2;
        // Triangle 2: v0, v2, v3
        indices[base_index + 3] = base_vertex + 0;
        indices[base_index + 4] = base_vertex + 2;
        indices[base_index + 5] = base_vertex + 3;
    }

    // Create index buffer
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_DYNAMIC_DRAW);

    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    // Get the basic shader from renderer and set up uniforms
    auto& r = renderer();
    Shader* shader = nullptr;

    if (use_own_shader) {
        shader = r.getBasicShader();
        if (shader) {
            shader->bind();

            // Set MVP matrix
            GLint mvp_loc = shader->getUniformLocation("u_mvp");
            if (mvp_loc >= 0) {
                glm::mat4 mvp = r.getMVP();
                glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(mvp));
            }

            // Set texture usage flag
            GLint use_tex_loc = shader->getUniformLocation("u_use_texture");
            if (use_tex_loc >= 0) {
                glUniform1i(use_tex_loc, !textures.empty() ? 1 : 0);
            }

            // Set texture sampler
            GLint tex_loc = shader->getUniformLocation("u_texture");
            if (tex_loc >= 0) {
                glUniform1i(tex_loc, 0);
            }
        }
    }

    int last_index = vertex_count - 1;

    if(textures.empty()) {
        // Draw all triangles at once
        glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, 0);
    } else {
        // Draw with texture changes
        for(std::vector<quadbuf_tex>::iterator it = textures.begin(); it != textures.end();) {
            quadbuf_tex* tex = &(*it);
            int start_vertex = tex->start_index;
            int end_vertex;

            it++;

            if(it == textures.end()) {
                end_vertex = last_index;
            } else {
                end_vertex = (*it).start_index - 1;
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex->textureid);

            // Calculate which quads and indices to draw
            int start_quad = start_vertex / 4;
            int end_quad = end_vertex / 4;
            int num_quads_to_draw = end_quad - start_quad + 1;
            int start_index_offset = start_quad * 6;
            int indices_to_draw = num_quads_to_draw * 6;

            glDrawElements(GL_TRIANGLES, indices_to_draw, GL_UNSIGNED_SHORT,
                          (void*)(start_index_offset * sizeof(GLushort)));

            if(end_vertex >= last_index) break;
        }
    }

    if (use_own_shader && shader) {
        shader->unbind();
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &ebo);
}
