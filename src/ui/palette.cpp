#include "palette.h"
#include "node_canvas.h"
#include "node_registry.h"
#include "graph_io.h"
#include "widget.h"

// ── Palette ───────────────────────────────────────────────────────────────────

Palette::Palette(int canvasH, NodeCanvas& canvas) : nc(canvas) {
    auto& reg = NodeRegistry::get();

    // One button per registered node type, with a header at each category change.
    // Registration order in main.cpp already groups types by category.
    bool first = true;
    NodeCategory curCat{};
    for (auto& typeName : reg.typeOrder()) {
        auto* info = reg.find(typeName);
        if (!info) continue;
        if (first || info->category != curCat) {
            Button hdr;
            hdr.label = categoryName(info->category);
            hdr.kind  = Kind::Header;
            buttons.push_back(std::move(hdr));
            curCat = info->category;
            first  = false;
        }
        Button btn;
        btn.label  = info->label;
        btn.color  = categoryColor(info->category);
        btn.action = [this, typeName, &reg] {
            auto* info2 = reg.find(typeName);
            if (info2) nc.addNode(info2->create(nextSpawnPos()));
        };
        buttons.push_back(std::move(btn));
    }

    // Footer: Save / Load
    buttons.push_back({"Save", {}, [this]{ graphSave(nc, "graph.json"); }, Kind::Ghost});
    buttons.push_back({"Load", {}, [this]{ graphLoad(nc, "graph.json"); }, Kind::Ghost});

    resize(canvasH);
}

void Palette::resize(int canvasH) {
    panelH = (float)canvasH;
    layout();
}

void Palette::layout() {
    const float fullW = kW - kPadX * 2.f;

    // Node list, top-down
    float y = kPadY;
    for (auto& b : buttons) {
        if (b.kind == Kind::Ghost) continue;
        b.x = kPadX; b.y = y; b.w = fullW;
        y += (b.kind == Kind::Header ? kHeaderH : kBtnH) + kBtnGap;
    }

    // Footer row pinned to the bottom, but never above the end of the list
    footerY = std::max(y + kPadY, panelH - kPadY - kBtnH);
    const float halfW = (fullW - kBtnGap) * 0.5f;
    float x = kPadX;
    for (auto& b : buttons) {
        if (b.kind != Kind::Ghost) continue;
        b.x = x; b.y = footerY; b.w = halfW;
        x += halfW + kBtnGap;
    }
}

bool Palette::contains(float x, float y) const {
    return x < kW && y < panelH;
}

void Palette::onMouseMove(float x, float y) {
    hoveredIdx = contains(x, y) ? buttonAt(x, y) : -1;
}

bool Palette::onMouseDown(float x, float y) {
    if (!contains(x, y)) return false;
    int idx = buttonAt(x, y);
    if (idx >= 0 && buttons[idx].action)
        buttons[idx].action();
    return true;
}

int Palette::buttonAt(float x, float y) const {
    for (int i = 0; i < (int)buttons.size(); ++i) {
        const auto& b = buttons[i];
        if (b.kind == Kind::Header) continue;
        if (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + kBtnH) return i;
    }
    return -1;
}

Vec2 Palette::nextSpawnPos() {
    Vec2 p = spawnCursor;
    spawnCursor.x += 20.f;
    spawnCursor.y += 20.f;
    if (spawnCursor.y > 500.f) spawnCursor = {kW + 20.f, 60.f};
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
    const auto& btn = buttons[idx];
    const float textY = btn.y + kBtnH * 0.5f - 7.f;

    if (btn.kind == Kind::Ghost) {
        // Outlined, transparent unless hovered; label roughly centred
        auto sh = tvg::Shape::gen();
        sh->appendRect(btn.x, btn.y, btn.w, kBtnH, kCorner, kCorner);
        if (hovered) sh->fill(pal::PalBtnHover.r, pal::PalBtnHover.g, pal::PalBtnHover.b, pal::PalBtnHover.a);
        else         sh->fill(0, 0, 0, 0);
        sh->stroke(1.f);
        sh->stroke(pal::PalGhost.r, pal::PalGhost.g, pal::PalGhost.b, pal::PalGhost.a);
        scene.push(std::move(sh));
        float textW = btn.label.size() * 6.f;  // approx width at 11pt
        pushText(scene, btn.label, btn.x + (btn.w - textW) * 0.5f, textY, 11.f, pal::PalGhostText);
        return;
    }

    // Dark neutral fill + category-coloured stripe on the left edge
    Color4 bg = hovered ? pal::PalBtnHover : pal::PalBtn;
    auto sh = tvg::Shape::gen();
    sh->appendRect(btn.x, btn.y, btn.w, kBtnH, kCorner, kCorner);
    sh->fill(bg.r, bg.g, bg.b, bg.a);
    scene.push(std::move(sh));

    auto stripe = tvg::Shape::gen();
    stripe->appendRect(btn.x, btn.y, kStripeW, kBtnH, 2.f, 2.f);
    stripe->fill(btn.color.r, btn.color.g, btn.color.b, btn.color.a);
    scene.push(std::move(stripe));

    pushText(scene, btn.label, btn.x + kStripeW + 7.f, textY, 11.f, pal::PalBtnText);
}

void Palette::draw(tvg::Scene& scene) const {
    // Panel background
    {
        auto sh = tvg::Shape::gen();
        sh->appendRect(0, 0, kW, panelH, 0, 0);
        sh->fill(pal::PalBg.r, pal::PalBg.g, pal::PalBg.b, pal::PalBg.a);
        scene.push(std::move(sh));
        // Right border
        auto border = tvg::Shape::gen();
        border->moveTo(kW, 0);
        border->lineTo(kW, panelH);
        border->stroke(1.f);
        border->stroke(pal::PalBorder.r, pal::PalBorder.g, pal::PalBorder.b, pal::PalBorder.a);
        border->fill(0,0,0,0);
        scene.push(std::move(border));
    }

    for (int i = 0; i < (int)buttons.size(); ++i) {
        const auto& b = buttons[i];
        if (b.kind == Kind::Header) {
            // Small dim category label
            pushText(scene, b.label, kPadX + 2.f, b.y, 9.f, pal::PalHeader);
        } else {
            drawButton(scene, i, i == hoveredIdx);
        }
    }

    // Divider above the footer
    {
        float sy = footerY - kPadY * 0.5f;
        auto sh = tvg::Shape::gen();
        sh->moveTo(kPadX, sy);
        sh->lineTo(kW - kPadX, sy);
        sh->stroke(1.f);
        sh->stroke(pal::PalBorder.r, pal::PalBorder.g, pal::PalBorder.b, 160);
        sh->fill(0, 0, 0, 0);
        scene.push(std::move(sh));
    }
}
