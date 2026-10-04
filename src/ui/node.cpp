#include "node.h"
#include "node_canvas.h"

SocketWidget* Node::addInput(std::string label) {
    auto s = std::make_unique<SocketWidget>(SocketDir::Input, sockInputCount, std::move(label));
    float y = kSockOffY + sockInputCount * kSockSpaceY;
    s->localPos = {-SocketWidget::kR, y};
    ++sockInputCount;
    // Grow height if sockets extend beyond current body
    float needed = s->localPos.y + SocketWidget::kR * 2.f + kPadY;
    if (needed > height) height = needed;
    auto* raw = s.get();
    addChild(std::move(s));
    return raw;
}

SocketWidget* Node::addOutput(std::string label) {
    auto s = std::make_unique<SocketWidget>(SocketDir::Output, sockOutputCount, std::move(label));
    float y = kSockOffY + sockOutputCount * kSockSpaceY;
    s->localPos = {width - SocketWidget::kR, y};
    ++sockOutputCount;
    float needed = s->localPos.y + SocketWidget::kR * 2.f + kPadY;
    if (needed > height) height = needed;
    auto* raw = s.get();
    addChild(std::move(s));
    return raw;
}

void Node::draw(tvg::Scene& scene) {
    Vec2 wp = worldPos();

    // Shadow (subtle, offset)
    pushRect(scene, wp.x+4, wp.y+4, width, height, kCornerR,
             {0,0,0,60});

    // Body
    pushRect(scene, wp.x, wp.y, width, height, kCornerR,
             pal::NodeBg, pal::NodeBorder, 1.5f);

    // Title bar (rounded top corners only — approximated by full-corner rect clipped by body)
    Color4 titleCol = selected ? pal::TitleBgSel : pal::TitleBg;
    pushRect(scene, wp.x, wp.y, width, kTitleH, kCornerR, titleCol);
    // Cover bottom-round of title bar with a square-bottomed rect
    pushRect(scene, wp.x, wp.y + kTitleH * 0.5f, width, kTitleH * 0.5f, 0.f, titleCol);

    // Title text
    if (NodeCanvas::fontLoaded() && !title.empty()) {
        auto t = tvg::Text::gen();
        if (t->font(NodeCanvas::fontName().c_str(), 11.f) == tvg::Result::Success) {
            t->text(title.c_str());
            t->fill(220, 220, 235);
            t->translate(wp.x + kPadX, wp.y + kTitleH * 0.5f + 4.f);
            scene.push(std::move(t));
        }
    }

    // Socket labels
    if (NodeCanvas::fontLoaded()) {
        for (auto& child : children) {
            auto* s = dynamic_cast<SocketWidget*>(child.get());
            if (!s || s->label.empty()) continue;
            auto t = tvg::Text::gen();
            if (t->font(NodeCanvas::fontName().c_str(), 9.f) != tvg::Result::Success) break;
            t->text(s->label.c_str());
            t->fill(150, 150, 170);
            Vec2 c = s->worldCenter();
            if (s->dir == SocketDir::Input) {
                t->translate(c.x + SocketWidget::kR + 3.f, c.y + 4.f);
            } else {
                // Approximate label width at 9pt (~5.5px per char) to right-align
                float labelW = s->label.size() * 5.5f;
                t->translate(c.x - SocketWidget::kR - 3.f - labelW, c.y + 4.f);
            }
            scene.push(std::move(t));
        }
    }
}
