// === File: src/core/ui/solid_layout.cpp =======================================
// AGENT: PURPOSE    — Solid background layout for UI panels
// AGENT: STATUS     — converted for WebGL (2026-01-09)
// =============================================================================

#include "solid_layout.h"
#include "../renderer.h"

UISolidLayout::UISolidLayout(bool horizontal) : UILayout(horizontal) {

    inverted   = false;

    bgtex.resize(4);

    bgtex[0] = texturemanager.grab("ui/layout_tl.png", false);
    bgtex[1] = texturemanager.grab("ui/layout_tr.png", false);
    bgtex[2] = texturemanager.grab("ui/layout_br.png", false);
    bgtex[3] = texturemanager.grab("ui/layout_bl.png", false);

    for(TextureResource* t: bgtex) {
        t->bind();
        t->setFiltering(GL_NEAREST, GL_NEAREST);
        t->setWrapStyle(GL_CLAMP_TO_EDGE);
    }
}

UISolidLayout::~UISolidLayout() {
    for(TextureResource* t: bgtex) {
        texturemanager.release(t);
    }
    bgtex.clear();
}

void UISolidLayout::drawBackground() {

    auto& r = renderer();

    r.pushModelView();
    r.translateMV(pos.x, pos.y, 0.0f);

    r.color(ui->getSolidColour());

    ui->setTextured(true);

    vec4 texcoord;

    vec2 rect = getRect();

    for(int i=0; i < 4; i++) {

        r.pushModelView();

        if(inverted) {
            bgtex[(i+2)%4]->bind();

            switch(i) {
                case 0:
                    texcoord = vec4(1.0f, 1.0f, 1.0-(rect.x/32.0f), 1.0-(rect.y/32.0f));
                    break;
                case 1:
                    texcoord = vec4((rect.x/32.0f), 1.0f, 0.0, 1.0-(rect.y/32.0f));
                    r.translateMV(rect.x*0.5f, 0.0f, 0.0f);
                    break;
                case 2:
                    texcoord = vec4(rect.x/32.0f, rect.y/32.0f, 0.0f, 0.0f);
                    r.translateMV(rect.x*0.5f, rect.y*0.5f, 0.0f);
                    break;
                case 3:
                    texcoord = vec4(1.0f, (rect.y/32.0f), 1.0f-(rect.x/32.0f), 0.0f);
                    r.translateMV(0.0f, rect.y*0.5f, 0.0f);
                    break;
            }
        } else {
            bgtex[i]->bind();

            switch(i) {
                case 0:
                    texcoord = vec4(0.0f, 0.0f, rect.x/32.0f, rect.y/32.0f);
                    break;
                case 1:
                    texcoord = vec4(1.0f-(rect.x/32.0f), 0.0f, 1.0f, (rect.y/32.0f));
                    r.translateMV(rect.x*0.5f, 0.0f, 0.0f);
                    break;
                case 2:
                    texcoord = vec4(1.0-rect.x/32.0f, 1.0-rect.y/32.0f, 1.0f, 1.0f);
                    r.translateMV(rect.x*0.5f, rect.y*0.5f, 0.0f);
                    break;
                case 3:
                    texcoord = vec4(0.0, 1.0-(rect.y/32.0f), (rect.x/32.0f), 1.0f);;
                    r.translateMV(0.0f, rect.y*0.5f, 0.0f);
                    break;
            }
        }

        r.begin(GL_QUADS);
            r.texcoord(texcoord.x, texcoord.y);
            r.vertex(0.0f, 0.0f);

            r.texcoord(texcoord.z, texcoord.y);
            r.vertex(rect.x*0.5f, 0.0f);

            r.texcoord(texcoord.z, texcoord.w);
            r.vertex(rect.x*0.5f, rect.y*0.5f);

            r.texcoord(texcoord.x, texcoord.w);
            r.vertex(0.0f, rect.y*0.5f);
        r.end();

        r.popModelView();
    }

    r.popModelView();
}
