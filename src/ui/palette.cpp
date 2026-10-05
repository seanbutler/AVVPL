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

    Color4 bg = hovered
        ? Color4{(uint8_t)std::min(255, btn.color.r + 30),
                 (uint8_t)std::min(255, btn.color.g + 30),
                 (uint8_t)std::min(255, btn.color.b + 30), 255}
        : btn.color;

    auto sh = tvg::Shape::gen();
    sh->appendRect(bx, by, bw, kBtnH, kCorner, kCorner);
    sh->fill(bg.r, bg.g, bg.b, bg.a);
    scene.push(std::move(sh));

    // Label — baseline sits roughly at the vertical centre of the button.
    pushText(scene, btn.label, bx + 8.f, by + kBtnH * 0.5f - 2.f, 11.f, {220, 220, 235, 255});
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
