// === File: src/core/ui/element.cpp ============================================
// AGENT: PURPOSE    — UI element base class (updated for WebGL)
// AGENT: STATUS     — in-progress (2026-01-09)
// =============================================================================

#include "element.h"
#include "../renderer.h"

std::map<int, std::string> element_names = {
    {UI_INVALID, "Invalid"},
    {UI_ELEMENT, "Element"},
    {UI_LABEL, "Label"},
    {UI_BUTTON, "Button"},
    {UI_IMAGE, "Image"},
    {UI_LAYOUT, "Layout"},
    {UI_GROUP, "Group"},
    {UI_COLOUR, "Colour"},
    {UI_SELECT, "Select"},
    {UI_SLIDER, "Slider"},
    {UI_SCROLL_BAR, "ScrollBar"},
    {UI_CHECKBOX, "Checkbox"}
};

UIElement::~UIElement() {
    if (ui && selected) ui->deselect();
}

const std::string& UIElement::getElementName() const {
    return getElementName(this->getType());
}

const std::string& UIElement::getElementName(int type) {
    auto it = element_names.find(type);
    if (it != element_names.end()) return it->second;
    return element_names[UI_INVALID];
}

void UIElement::drawOutline() {
    ui->setTextured(false);

    auto& r = renderer();
    r.pushModelView();
    r.translateMV(0.5f, 0.5f, 0.0f);
    r.color(1.0f, 1.0f, 1.0f, 1.0f);

    r.begin(GL_LINE_LOOP);
    r.vertex(0.0f, 0.0f);
    r.vertex(rect.x, 0.0f);
    r.vertex(rect.x, rect.y);
    r.vertex(0.0f, rect.y);
    r.end();

    r.popModelView();
    ui->setTextured(true);
}

void UIElement::getModifiers(bool& left_ctrl, bool& left_shift) {
    left_ctrl = left_shift = false;

    const Uint8* keystate = SDL_GetKeyboardState(NULL);
    if (keystate[SDL_SCANCODE_LCTRL]) left_ctrl = true;
    if (keystate[SDL_SCANCODE_LSHIFT]) left_shift = true;
}

double UIElement::granulaity(double initial, double scale) {
    bool left_ctrl, left_shift;
    getModifiers(left_ctrl, left_shift);

    double granul = initial;
    if (left_ctrl) granul *= scale;
    if (left_shift) granul *= scale;

    return granul;
}

void UIElement::drawOutline(const vec2& rect) {
    auto& r = renderer();
    r.pushModelView();
    r.translateMV(0.5f, 0.5f, 0.0f);

    r.begin(GL_LINE_LOOP);
    r.vertex(0.0f, 0.0f);
    r.vertex(rect.x, 0.0f);
    r.vertex(rect.x, rect.y);
    r.vertex(0.0f, rect.y);
    r.end();

    r.popModelView();
}

void UIElement::drawQuad(const vec2& pos, const vec2& rect, const vec4& texcoord) {
    auto& r = renderer();
    r.pushModelView();
    r.translateMV(pos.x, pos.y, 0.0f);
    drawQuad(rect, texcoord);
    r.popModelView();
}

void UIElement::drawQuad(const vec2& rect, const vec4& texcoord) {
    auto& r = renderer();
    r.begin(GL_QUADS);
    r.texcoord(texcoord.x, texcoord.y);
    r.vertex(0.0f, 0.0f);
    r.texcoord(texcoord.z, texcoord.y);
    r.vertex(rect.x, 0.0f);
    r.texcoord(texcoord.z, texcoord.w);
    r.vertex(rect.x, rect.y);
    r.texcoord(texcoord.x, texcoord.w);
    r.vertex(0.0f, rect.y);
    r.end();
}

void UIElement::draw() {
    auto& r = renderer();
    r.pushModelView();
    r.translateMV(pos.x, pos.y, 0.0f);
    drawContent();
    r.popModelView();
}

void UIElement::update(float dt) {
    updateContent();
    updateRect();
    updateZIndex();
}

void UIElement::updateZIndex() {
    if (parent != nullptr) zindex = parent->zindex + 1;
}

void UIElement::scroll(bool up) {
    if (parent != nullptr) parent->scroll(up);
}

bool UIElement::elementsByType(std::list<UIElement*>& found, int type) {
    if (getType() == type) {
        found.push_back(this);
        return true;
    }
    return false;
}

void UIElement::elementsAt(const vec2& pos, std::list<UIElement*>& elements_found) {
    if (hidden) return;

    vec2 rect = getRect();

    if (pos.x >= this->pos.x && pos.x <= (this->pos.x + rect.x) &&
        pos.y >= this->pos.y && pos.y <= (this->pos.y + rect.y)) {
        elements_found.push_back(this);
    }
}
