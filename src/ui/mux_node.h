#pragma once
#include "node.h"
#include "socket.h"
#include <array>

// ── MuxDisplayWidget ──────────────────────────────────────────────────────────
// Row of kCount cells; lights up the active input slot.

class MuxDisplayWidget final : public Widget {
public:
    static constexpr int   kCount = 4;
    static constexpr float kH     = 20.f;
    static constexpr float kGap   =  4.f;

    int active = 0;

    MuxDisplayWidget() { height = kH; }

    bool contains(Vec2 pt) const override {
        Vec2 wp = worldPos();
        return pt.x >= wp.x && pt.x <= wp.x + width
            && pt.y >= wp.y && pt.y <= wp.y + height;
    }

    void draw(tvg::Scene& scene) override {
        Vec2  wp    = worldPos();
        float cellW = (width - (kCount - 1) * kGap) / (float)kCount;
        pushRect(scene, wp.x, wp.y, width, height, 3.f, {18, 18, 26, 255});
        for (int i = 0; i < kCount; ++i) {
            float cx = wp.x + i * (cellW + kGap);
            bool  on = (i == active);
            pushRect(scene, cx, wp.y, cellW, height, 3.f,
                     on ? Color4{100, 180, 255, 255} : Color4{28, 28, 40, 255});
        }
    }
};

// ── MuxNode ───────────────────────────────────────────────────────────────────
// Sequential switch: forwards the selected input's CV to the output.
// Each trigger on Next advances to the next input (wraps around).
// Equivalent to a Doepfer A-151 sequential switch.

class MuxNode final : public Node {
public:
    static constexpr int kCount = 4;

    std::array<SocketWidget*, kCount> inNext = {};
    std::array<SocketWidget*, kCount> inCV   = {};
    SocketWidget* outCV = nullptr;

    explicit MuxNode(Vec2 pos = {}) : Node("Mux", pos) {
        for (int i = 0; i < kCount; ++i) {
            inNext[i] = addInput("Next " + std::to_string(i + 1), SocketType::Trigger);
            inCV[i]   = addInput("In "   + std::to_string(i + 1), SocketType::CV);
        }

        display = addWidget(std::make_unique<MuxDisplayWidget>());

        outCV = addOutput("Out", SocketType::CV);
    }

    void tick(float /*dt*/) override {
        if (inNext[step]->value.trig)
            step = (step + 1) % kCount;

        outCV->value.f  = inCV[step]->value.f;
        display->active = step;
    }

private:
    MuxDisplayWidget* display = nullptr;
    int               step    = 0;
};
