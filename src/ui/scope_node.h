#pragma once
#include "node.h"
#include "tone_node.h"
#include "slider.h"
#include <thorvg.h>
#include <cmath>
#include <algorithm>

// ── ScopeDisplayWidget ────────────────────────────────────────────────────────
// Oscilloscope view of the summed output of every ToneNode in the graph.
// Triggers on a rising zero crossing so periodic waveforms stand still.
class ScopeDisplayWidget final : public Widget {
public:
    static constexpr float kH       = 90.f;
    static constexpr int   kHistory = ToneNode::kTapSize / 2;
    static constexpr int   kMaxWindow = 4000; // ~90ms at 44.1kHz; leaves room to search for a trigger

    int window = 441;   // samples shown, set by ScopeNode from its Time slider

    ScopeDisplayWidget() { height = kH; }

    bool contains(Vec2) const override { return false; }
    void onDrag(Vec2)         override {}

    void draw(tvg::Scene& scene) override {
        Vec2  wp = worldPos();
        float cy = wp.y + kH * 0.5f;
        float halfH = kH * 0.5f - 3.f;

        pushRect(scene, wp.x, wp.y, width, kH, 6.f, {30, 30, 40, 255});
        pushLine(scene, wp.x + 4.f, cy, wp.x + width - 4.f, cy, {60, 60, 78, 255}, 1.f);

        std::fill(std::begin(buf), std::end(buf), 0.f);
        for (auto* t : ToneNode::instances()) t->mixRecent(buf, kHistory);

        int n = std::clamp(window, 2, kMaxWindow);

        // Search backwards from the newest possible window start for a rising zero crossing.
        int start = kHistory - n;
        for (int i = start; i > 0; --i) {
            if (buf[i - 1] < 0.f && buf[i] >= 0.f) { start = i; break; }
        }

        bool clipping = false;
        auto sh = tvg::Shape::gen();
        for (int i = 0; i < n; ++i) {
            float s = buf[start + i];
            if (std::abs(s) > 1.f) clipping = true;
            float x = wp.x + (float)i / (n - 1) * width;
            float y = cy - clampf(s, -1.f, 1.f) * halfH;
            if (i == 0) sh->moveTo(x, y);
            else        sh->lineTo(x, y);
        }
        sh->fill(0, 0, 0, 0);
        sh->stroke(1.5f);
        if (clipping) sh->stroke(230, 90, 80, 255);
        else          sh->stroke(140, 200, 255, 255);
        scene.push(std::move(sh));
    }

private:
    float buf[kHistory];
};

// ── ScopeNode ─────────────────────────────────────────────────────────────────
// No sockets: it listens to the whole graph's audio output.
// The Time slider sets the horizontal span of the display in milliseconds.
class ScopeNode final : public Node {
public:
    explicit ScopeNode(Vec2 pos = {}) : Node("Scope", pos) {
        display = addWidget(std::make_unique<ScopeDisplayWidget>());
        timeMs  = addWidget(std::make_unique<SliderWidget>("Time ms", 1.f, 90.f, 10.f));
    }

    // Read the slider directly (rather than via onChange) so loaded values apply too.
    void tick(float /*dt*/) override {
        display->window = (int)std::lround(timeMs->value * 0.001f * ToneNode::kSampleRate);
    }

private:
    ScopeDisplayWidget* display = nullptr;
    SliderWidget*       timeMs  = nullptr;
};
