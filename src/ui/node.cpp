#include "node.h"

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
}
