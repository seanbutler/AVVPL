#pragma once
#include "widget.h"
#include "socket.h"
#include <string>

// A draggable container node with:
//   - a title bar (drag handle)
//   - input sockets on the left edge
//   - output sockets on the right edge
//   - a body area where child widgets (sliders, dials, histograms) are stacked vertically
class Node : public Widget {
public:
    std::string title;
    bool        selected = false;
    Color4      accent   {0, 0, 0, 0};  // category colour stripe under the title; set by NodeRegistry

    static constexpr float kCornerR    =  0.f;
    static constexpr float kTitleH     = 24.f;
    static constexpr float kPadX       = 10.f;
    static constexpr float kPadY       =  6.f;
    static constexpr float kSockSpaceY = 22.f;  // vertical gap between sockets
    static constexpr float kSockOffY   = kTitleH + 10.f;  // first socket Y offset
    static constexpr float kMinWidth   = 180.f;
    static constexpr float kAccentH    =  3.f;   // category stripe height

    explicit Node(std::string t, Vec2 pos = {})
        : title(std::move(t))
    { localPos = pos; width = kMinWidth; height = kTitleH + kPadY; }

    // Add an input socket and return a raw pointer for wiring.
    SocketWidget* addInput (std::string label, SocketType type = SocketType::CV);
    SocketWidget* addOutput(std::string label, SocketType type = SocketType::CV);

    // Add a body widget; adjusts height automatically.
    // Returns raw pointer for setting callbacks.
    template<typename T>
    T* addWidget(std::unique_ptr<T> w) {
        w->width    = width - 2.f * kPadX;
        w->localPos = {kPadX, height};
        height     += w->height + kPadY;
        auto* raw   = w.get();
        addChild(std::move(w));
        return raw;
    }

    // Hit-test: only count as a drag candidate when the title bar is hit.
    bool titleBarHit(Vec2 worldPt) const {
        Vec2 wp = worldPos();
        return worldPt.x >= wp.x && worldPt.x <= wp.x + width
            && worldPt.y >= wp.y && worldPt.y <= wp.y + kTitleH;
    }

    // Called each frame by NodeCanvas::tick(). Override in subclasses to
    // read input socket values, update internal state, write output socket values.
    virtual void tick(float /*dt*/) {}

    // Draw node body + children first, then socket labels on top.
    void drawAll(tvg::Scene& scene) override;

    bool contains(Vec2 worldPt) const override {
        Vec2 wp = worldPos();
        return worldPt.x >= wp.x && worldPt.x <= wp.x + width
            && worldPt.y >= wp.y && worldPt.y <= wp.y + height;
    }

    void draw(tvg::Scene& scene) override;

private:
    int sockInputCount  = 0;
    int sockOutputCount = 0;

    void layoutSockets();
};
