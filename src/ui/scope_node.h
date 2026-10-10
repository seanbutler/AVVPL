#pragma once
#include "node.h"
#include "socket.h"
#include "slider.h"
#include <thorvg.h>
#include <algorithm>
#include <array>
#include <cmath>

// ── ScopeDisplayWidget ────────────────────────────────────────────────────────
// Plots a rolling history of a float value pushed each frame.
// Auto-scales vertically to fit whatever signal is connected.
class ScopeDisplayWidget final : public Widget {
public:
    static constexpr float kH       = 90.f;
    static constexpr int   kBufSize = 512;

    int window = 128;  // how many frames of history to display

    ScopeDisplayWidget() { height = kH; }

    void push(float v) {
        buf[writePos] = v;
        writePos = (writePos + 1) % kBufSize;
    }

    bool contains(Vec2) const override { return false; }
    void onDrag(Vec2)         override {}

    void draw(tvg::Scene& scene) override {
        Vec2  wp   = worldPos();
        int   n    = std::clamp(window, 2, kBufSize);

        pushRect(scene, wp.x, wp.y, width, kH, 0.f, {30, 30, 40, 255});

        // Find range over the visible window for auto-scaling
        float lo = buf[writePos % kBufSize], hi = lo;
        for (int i = 0; i < n; ++i) {
            float v = buf[(writePos - n + i + kBufSize) % kBufSize];
            lo = std::min(lo, v);
            hi = std::max(hi, v);
        }
        float span = hi - lo;
        if (span < 1e-6f) span = 1.f;  // flat signal — show it centred

        // Zero-line if 0 is within the visible range
        if (lo <= 0.f && hi >= 0.f) {
            float zy = wp.y + kH - 3.f - (-lo / span) * (kH - 6.f);
            pushLine(scene, wp.x + 4.f, zy, wp.x + width - 4.f, zy, {60, 60, 78, 255}, 1.f);
        }

        auto sh = tvg::Shape::gen();
        for (int i = 0; i < n; ++i) {
            float v = buf[(writePos - n + i + kBufSize) % kBufSize];
            float t = (v - lo) / span;  // 0–1
            float x = wp.x + (float)i / (float)(n - 1) * width;
            float y = wp.y + kH - 3.f - t * (kH - 6.f);
            if (i == 0) sh->moveTo(x, y);
            else        sh->lineTo(x, y);
        }
        sh->fill(0, 0, 0, 0);
        sh->stroke(1.5f);
        sh->stroke(140, 200, 255, 255);
        scene.push(std::move(sh));
    }

private:
    std::array<float, kBufSize> buf      = {};
    int                         writePos = 0;
};

// ── ScopeNode ─────────────────────────────────────────────────────────────────
// Graphs whatever is plugged into In and passes it through unchanged to Out.
class ScopeNode final : public Node {
public:
    SocketWidget* inSignal  = nullptr;
    SocketWidget* outSignal = nullptr;

    explicit ScopeNode(Vec2 pos = {}) : Node("Scope", pos) {
        inSignal = addInput("In", SocketType::CV);

        display      = addWidget(std::make_unique<ScopeDisplayWidget>());
        windowSlider = addWidget(std::make_unique<SliderWidget>("Window", 8.f, 512.f, 128.f));

        outSignal = addOutput("Out", SocketType::CV);
    }

    void tick(float /*dt*/) override {
        float v = inSignal->value.f;
        display->push(v);
        display->window  = (int)windowSlider->value;
        outSignal->value.f = v;
    }

private:
    ScopeDisplayWidget* display      = nullptr;
    SliderWidget*       windowSlider = nullptr;
};
