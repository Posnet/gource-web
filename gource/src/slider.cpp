// === File: src/slider.cpp ====================================================
// AGENT: PURPOSE    — Position slider UI widget
// AGENT: STATUS     — converted for WebGL (2026-01-09)
// =============================================================================
/*
    Copyright (C) 2009 Andrew Caudwell (acaudwell@gmail.com)

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

#include "slider.h"
#include "gource_settings.h"
#include "core/renderer.h"

// PositionSlider

PositionSlider::PositionSlider(float percent) {
    this->percent = percent;
}

void PositionSlider::init() {
    font = fontmanager.grab(gGourceSettings.font_file, 16 * gGourceSettings.font_scale);
    font.dropShadow(true);

    slidercol = vec3(1.0, 1.0, 1.0);

    mouseover = -1.0;

    mouseover_elapsed = 1.0;
    fade_time = 1.0;
    alpha = 0.0;

    capwidth = 0.0f;

    resize();
}

const Bounds2D& PositionSlider::getBounds() const {
    return bounds;
}

void PositionSlider::resize() {
    int gap = 35;

    bounds.reset();
    bounds.update(vec2(gap, display.height - gap*2));
    bounds.update(vec2(display.width - gap, display.height - gap));
}

void PositionSlider::setColour(vec3 col) {
    slidercol = col;
}

void PositionSlider::show() {
    mouseover_elapsed = 0.0;
}

bool PositionSlider::mouseOver(vec2 pos, float* percent_ptr) {
    if(bounds.contains(pos)) {

        mouseover_elapsed = 0;
        mouseover = pos.x;

        if(percent_ptr != 0) {
            *percent_ptr = (float) (pos.x - bounds.min.x) / (bounds.max.x - bounds.min.x);
        }

        return true;
    }

    mouseover = -1.0;

    return false;
}

bool PositionSlider::click(vec2 pos, float* percent_ptr) {
    if(mouseOver(pos, &percent)) {

        if(percent_ptr != 0) {
            *percent_ptr = percent;
        }

        return true;
    }

    return false;
}

void PositionSlider::setCaption(const std::string& caption) {
    capwidth = 0.0;
    this->caption = caption;

    if(caption.size()) {
        capwidth = font.getWidth(caption.c_str());
    }
}

void PositionSlider::setPercent(float percent) {
    this->percent = percent;
}

void PositionSlider::logic(float dt) {

    if(mouseover < 0.0 && mouseover_elapsed < fade_time) mouseover_elapsed += dt;

    if(mouseover_elapsed < fade_time && alpha < 1.0) {
        alpha = std::min(1.0f, alpha+dt);

    } else if(mouseover_elapsed >= fade_time && alpha > 0.0) {

        alpha = std::max(0.0f, alpha-dt);

    }
}

void PositionSlider::drawSlider(float pos_x) const {
    auto& r = renderer();

    glLineWidth(2.0f);

    bounds.draw();

    glLineWidth(2.0f);

    r.begin(GL_LINES);
        r.vertex(pos_x, bounds.min.y);
        r.vertex(pos_x, bounds.max.y);
    r.end();
}

void PositionSlider::draw(float dt) {
    auto& r = renderer();

    glDisable(GL_TEXTURE_2D);
    r.unbindTexture();

    float pos_x = bounds.min.x + (bounds.max.x - bounds.min.x) * percent;

    r.color(0.0f, 0.0f, 0.0f, 0.7f*alpha);

    r.pushModelView();
    r.translateMV(2.0f, 2.0f, 0.0f);
    drawSlider(pos_x);
    r.popModelView();

    r.color(slidercol.x, slidercol.y, slidercol.z, alpha);

    drawSlider(pos_x);

    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);

    r.color(1.0f, 1.0f, 1.0f, 1.0f);

    if(caption.size() && mouseover >= 0.0) {
        int height_offset = 25 * gGourceSettings.font_scale;
        font.draw(std::min((double)display.width - capwidth - 1.0, std::max(1.0, mouseover - (capwidth/2.0))), bounds.min.y - height_offset, caption);
    }

}

