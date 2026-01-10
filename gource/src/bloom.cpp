// === File: src/bloom.cpp =====================================================
// AGENT: PURPOSE    — Bloom effect vertex buffer implementation
// AGENT: STATUS     — converted for WebGL (2026-01-09)
// =============================================================================
/*
    Copyright (C) 2011 Andrew Caudwell (acaudwell@gmail.com)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "bloom.h"
#include "core/renderer.h"

bloombuf::bloombuf(int data_size) : buffer_size(0), vertex_count(0), vao(0), bufferid(0), vao_initialized(false) {
    if (data_size > 0) {
        data.reserve(data_size);
    }
}

bloombuf::~bloombuf() {
    unload();
}

void bloombuf::unload() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (bufferid != 0) {
        glDeleteBuffers(1, &bufferid);
        bufferid = 0;
    }
    buffer_size = 0;
    vao_initialized = false;
}

void bloombuf::reset() {
    vertex_count = 0;
    data.clear();
    triangles.clear();
}

size_t bloombuf::vertices() {
    return vertex_count;
}

size_t bloombuf::capacity() {
    return data.capacity();
}

void bloombuf::add(GLuint textureid, const vec2& pos, const vec2& dims, const vec4& colour, const vec4& texcoord) {
    // Create 4 vertices for a quad
    bloom_vertex v1(pos,                       colour, texcoord);
    bloom_vertex v2(pos + vec2(dims.x, 0.0f), colour, texcoord);
    bloom_vertex v3(pos + dims,                colour, texcoord);
    bloom_vertex v4(pos + vec2(0.0f, dims.y), colour, texcoord);

    data.push_back(v1);
    data.push_back(v2);
    data.push_back(v3);
    data.push_back(v4);

    vertex_count += 4;
}

void bloombuf::convertQuadsToTriangles() {
    triangles.clear();
    if (data.empty()) return;

    // Reserve space for triangles (6 vertices per quad instead of 4)
    triangles.reserve((data.size() / 4) * 6);

    // Convert each quad (4 vertices) to 2 triangles (6 vertices)
    for (size_t i = 0; i + 3 < data.size(); i += 4) {
        const bloom_vertex& v0 = data[i];
        const bloom_vertex& v1 = data[i + 1];
        const bloom_vertex& v2 = data[i + 2];
        const bloom_vertex& v3 = data[i + 3];

        // First triangle: v0, v1, v2
        triangles.push_back(v0);
        triangles.push_back(v1);
        triangles.push_back(v2);

        // Second triangle: v0, v2, v3
        triangles.push_back(v0);
        triangles.push_back(v2);
        triangles.push_back(v3);
    }
}

void bloombuf::setupVAO() {
    if (vao == 0) {
        glGenVertexArrays(1, &vao);
    }
    if (bufferid == 0) {
        glGenBuffers(1, &bufferid);
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, bufferid);

    // Position attribute (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(bloom_vertex),
                          (void*)offsetof(bloom_vertex, pos));

    // Color attribute (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(bloom_vertex),
                          (void*)offsetof(bloom_vertex, colour));

    // Texcoord attribute (location 2) - vec4 for bloom params
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(bloom_vertex),
                          (void*)offsetof(bloom_vertex, texcoord));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    vao_initialized = true;
}

void bloombuf::update() {
    if (vertex_count == 0) return;

    // Convert quads to triangles for WebGL compatibility
    convertQuadsToTriangles();

    if (!vao_initialized) {
        setupVAO();
    }

    glBindBuffer(GL_ARRAY_BUFFER, bufferid);

    size_t required_size = triangles.size() * sizeof(bloom_vertex);

    if (buffer_size < (int)triangles.size()) {
        buffer_size = triangles.size();
        glBufferData(GL_ARRAY_BUFFER, required_size, triangles.data(), GL_DYNAMIC_DRAW);
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, required_size, triangles.data());
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void bloombuf::draw() {
    if (triangles.empty() || vao == 0) return;

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, triangles.size());
    glBindVertexArray(0);
}
