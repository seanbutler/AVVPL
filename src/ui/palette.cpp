#include "palette.h"
#include "node_canvas.h"
#include "node_registry.h"
#include "graph_io.h"
#include "widget.h"

// ── Palette ───────────────────────────────────────────────────────────────────

Palette::Palette(int /*canvasH*/, NodeCanvas& canvas) : nc(canvas) {
    auto& reg = NodeRegistry::get();

    // One button per registered node type
    for (auto& typeName : reg.typeOrder()) {
        auto* info = reg.find(typeName);
        if (!info) continue;
        Button btn;
        btn.label  = info->label;
        btn.color  = info->color;
        btn.stub   = info->stub;
        btn.action = [this, typeName, &reg] {
            auto* info2 = reg.find(typeName);
            if (info2) nc.addNode(info2->create(nextSpawnPos()));
        };
        buttons.push_back(std::move(btn));
    }

    // Separator
    buttons.push_back({"", {}, {}, true});

    // Save
    buttons.push_back({"Save", {60, 140, 90, 255}, [this]{
        graphSave(nc, "graph.json");
    }});

    // Load
    buttons.push_back({"Load", {60, 90, 160, 255}, [this]{
        graphLoad(nc, "graph.json");
    }});

    panelH = kPadY + (float)buttons.size() * (kBtnH + kBtnGap);
}

bool Palette::contains(float x, float y) const {
    (void)y;
    return x < kW;
}

bool Palette::onMouseDown(float x, float y) {
    if (!contains(x, y)) return false;
    int idx = buttonAt(x, y);
    if (idx >= 0 && !buttons[idx].isSeparator && buttons[idx].action)
        buttons[idx].action();
    return true;
}

int Palette::buttonAt(float x, float y) const {
    if (x < kPadX || x > kW - kPadX) return -1;
    for (int i = 0; i < (int)buttons.size(); ++i) {
        if (buttons[i].isSeparator) continue;
        float by = btnY(i);
        if (y >= by && y <= by + kBtnH) return i;
    }
    return -1;
}

Vec2 Palette::nextSpawnPos() {
    Vec2 p = spawnCursor;
    spawnCursor.x += 20.f;
    spawnCursor.y += 20.f;
    if (spawnCursor.y > 500.f) spawnCursor = {130.f, 60.f};
    return p;
}

// ── Drawing ───────────────────────────────────────────────────────────────────

void Palette::pushText(tvg::Scene& scene, const std::string& text,
                       float x, float y, float size, Color4 color)
{
    if (!NodeCanvas::fontLoaded()) return;
    auto t = tvg::Text::gen();
    if (t->font(NodeCanvas::fontName().c_str(), size) != tvg::Result::Success) return;
    t->text(text.c_str());
    t->fill(color.r, color.g, color.b);
    t->translate(x, y);
    scene.push(std::move(t));
}

void Palette::drawButton(tvg::Scene& scene, int idx, bool hovered) const {
    auto& btn = buttons[idx];
    float by  = btnY(idx);
    float bx  = kPadX;
    float bw  = kW - kPadX * 2.f;

    // Stub buttons are dimmed; active buttons brighten on hover.
    Color4 base = btn.stub
        ? Color4{(uint8_t)(btn.color.r / 3), (uint8_t)(btn.color.g / 3),
                 (uint8_t)(btn.color.b / 3), 200}
        : btn.color;
    Color4 bg = (!btn.stub && hovered)
        ? Color4{(uint8_t)std::min(255, base.r + 30),
                 (uint8_t)std::min(255, base.g + 30),
                 (uint8_t)std::min(255, base.b + 30), 255}
        : base;

    auto sh = tvg::Shape::gen();
    sh->appendRect(bx, by, bw, kBtnH, kCorner, kCorner);
    sh->fill(bg.r, bg.g, bg.b, bg.a);
    scene.push(std::move(sh));

    // Label — baseline sits roughly at the vertical centre of the button.
    Color4 labelColor = btn.stub ? Color4{130, 130, 145, 180} : Color4{220, 220, 235, 255};
    pushText(scene, btn.label, bx + 8.f, by + kBtnH * 0.5f - 2.f, 11.f, labelColor);

    // Diagonal hatch lines to mark stub nodes as non-functional.
    if (btn.stub) {
        auto hatch = tvg::Shape::gen();
        hatch->moveTo(bx,        by);
        hatch->lineTo(bx + bw,   by + kBtnH);
        hatch->moveTo(bx,        by + kBtnH);
        hatch->lineTo(bx + bw,   by);
        hatch->stroke(1.f);
        hatch->stroke(80, 80, 95, 120);
        hatch->fill(0, 0, 0, 0);
        scene.push(std::move(hatch));
    }
}

void Palette::draw(tvg::Scene& scene) const {
    // Panel background
    {
        auto sh = tvg::Shape::gen();
        sh->appendRect(0, 0, kW, 9999.f, 0, 0);
        sh->fill(22, 22, 30, 245);
        scene.push(std::move(sh));
        // Right border
        auto border = tvg::Shape::gen();
        border->moveTo(kW, 0);
        border->lineTo(kW, 9999.f);
        border->stroke(1.f);
        border->stroke(55, 55, 72, 200);
        border->fill(0,0,0,0);
        scene.push(std::move(border));
    }

    // Title label
    pushText(scene, "Nodes", kPadX, 14.f, 10.f, {120, 120, 140, 255});

    for (int i = 0; i < (int)buttons.size(); ++i) {
        if (buttons[i].isSeparator) {
            // Thin divider line
            float sy = btnY(i) + kBtnH * 0.3f;
            auto sh = tvg::Shape::gen();
            sh->moveTo(kPadX, sy);
            sh->lineTo(kW - kPadX, sy);
            sh->stroke(1.f);
            sh->stroke(55, 55, 72, 160);
            sh->fill(0,0,0,0);
            scene.push(std::move(sh));
        } else {
            drawButton(scene, i, false);
        }
    }
}
