#pragma once
#include "node.h"
#include "socket.h"
#include <array>

// ── StepIndicatorWidget ───────────────────────────────────────────────────────
// Read-only row of 8 cells; lights up the active step.

class StepIndicatorWidget final : public Widget {
public:
    static constexpr int   kSteps = 16;
    static constexpr float kH     = 20.f;
    static constexpr float kGap   =  2.f;

    int activeStep = 0;

    StepIndicatorWidget() { height = kH; }

    bool contains(Vec2 pt) const override {
        Vec2 wp = worldPos();
        return pt.x >= wp.x && pt.x <= wp.x + width
            && pt.y >= wp.y && pt.y <= wp.y + height;
    }

    void draw(tvg::Scene& scene) override {
        Vec2  wp    = worldPos();
        float cellW = (width - (kSteps - 1) * kGap) / (float)kSteps;

        pushRect(scene, wp.x, wp.y, width, height, 3.f, {18, 18, 26, 255});

        for (int i = 0; i < kSteps; ++i) {
            float cx  = wp.x + i * (cellW + kGap);
            bool  act = (i == activeStep);
            pushRect(scene, cx, wp.y, cellW, height, 2.f,
                     act ? Color4{100, 200, 140, 255} : Color4{28, 28, 40, 255});
        }
    }
};

// ── StepRouterNode ────────────────────────────────────────────────────────────
// Advances one step per Trig input. Each step has its own Trig output that
// fires when the sequencer lands on that step.
// Reset input returns to step 0 without firing any output.

class StepRouterNode final : public Node {
public:
    static constexpr int kSteps = 16;

    SocketWidget* inTrig  = nullptr;
    SocketWidget* inReset = nullptr;
    std::array<SocketWidget*, kSteps> outTrig = {};

    explicit StepRouterNode(Vec2 pos = {}) : Node("Sequencer Router", pos) {
        width = 200.f;

        inTrig  = addInput("Trig");
        inReset = addInput("Reset");

        indicator = addWidget(std::make_unique<StepIndicatorWidget>());

        for (int i = 0; i < kSteps; ++i)
            outTrig[i] = addOutput(std::to_string(i + 1));
    }

    void tick(float /*dt*/) override {
        if (inReset->value.trig)
            step = 0;

        // Clear all outputs first
        for (int i = 0; i < kSteps; ++i)
            outTrig[i]->value.trig = false;

        if (inTrig->value.trig) {
            step = (step + 1) % kSteps;
            outTrig[step]->value.trig = true;
        }

        indicator->activeStep = step;
    }

private:
    StepIndicatorWidget* indicator = nullptr;
    int                  step      = 0;
};
