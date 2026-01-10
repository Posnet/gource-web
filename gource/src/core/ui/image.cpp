// === File: src/core/ui/image.cpp ==============================================
// AGENT: PURPOSE    — Image display UI element
// AGENT: STATUS     — converted for WebGL (2026-01-09)
// =============================================================================

#include "image.h"
#include "../renderer.h"

UIImage::UIImage(const std::string& image_path)
    : image_path(image_path), coords(0.0f, 0.0f, 1.0f, 1.0f) {

    rect = vec2(0.0f, 0.0f);
    init();
}

UIImage::UIImage(const std::string& image_path, const vec2& rect, const vec4& coords)
    : image_path(image_path), coords(coords) {

    this->rect = rect;

    init();
}

UIImage::~UIImage() {
    if(imagetex != 0) texturemanager.release(imagetex);
}

void UIImage::init() {
    imagetex = texturemanager.grab(image_path);

    colour        = vec4(1.0f);
    shadow_offset = vec2(1.0f, 1.0f);
    shadow        = 0.0f;

    if(glm::length(rect) < 1.0f) {
        rect = vec2(imagetex->w, imagetex->h);
    }
}

void UIImage::setTextureCoords(const vec4& coords) {
    this->coords = coords;
}

void UIImage::drawContent() {
    auto& r = renderer();

    imagetex->bind();

    if(shadow > 0.0f) {
        r.color(0.0f, 0.0f, 0.0f, shadow);
        drawQuad(rect + shadow_offset, coords);
    }

    r.color(colour);
    drawQuad(rect, coords);
}
