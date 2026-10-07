#pragma once
#include "node.h"
#include "socket.h"
#include "slider.h"
#include <algorithm>
#include <array>

// ── StepLaneWidget ────────────────────────────────────────────────────────────
// 8-cell step editor. Each cell is a vertical bar; drag up/down to set value.
// Dragging horizontally paints across cells.

class StepLaneWidget final : public Widget {
public:
    static constexpr int   kSteps = 8;
    static constexpr float kH     = 64.f;
    static constexpr float kGap   =  2.f;

    std::array<float, kSteps> values     = {};
    int                       activeStep = 0;

    StepLaneWidget() { height = kH; }

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
                     act ? Color4{45, 58, 80, 255} : Color4{28, 28, 40, 255});

            float fillH = values[i] * (height - 4.f);
            if (fillH > 0.5f) {
                pushRect(scene, cx + 1.f, wp.y + height - 2.f - fillH,
                         cellW - 2.f, fillH, 1.f,
                         act ? Color4{90, 175, 235, 255} : Color4{65, 120, 185, 210});
            }

            // Active step dot at top of cell
            if (act)
                pushCircle(scene, cx + cellW * 0.5f, wp.y + 5.f, 2.5f,
                           {200, 230, 255, 255});
        }
    }

    void onMouseDown(Vec2 pt, int) override {
        dragCell = cellAt(pt.x);
        if (dragCell >= 0) setValueAt(dragCell, pt.y);
    }

    void onMouseMove(Vec2 pt, Vec2) override {
        if (dragCell < 0) return;
        int c = cellAt(pt.x);
        if (c >= 0) dragCell = c;
        setValueAt(dragCell, pt.y);
    }

    void onMouseUp(Vec2, int) override { dragCell = -1; }

    nlohmann::json toJson() const override {
        nlohmann::json j = nlohmann::json::array();
        for (float v : values) j.push_back(v);
        return j;
    }

    void fromJson(const nlohmann::json& j) override {
        if (!j.is_array()) return;
        for (int i = 0; i < kSteps && i < (int)j.size(); ++i)
            if (j[i].is_number())
                values[i] = std::clamp(j[i].get<float>(), 0.f, 1.f);
    }

private:
    int dragCell = -1;

    int cellAt(float worldX) const {
        Vec2  wp    = worldPos();
        float cellW = (width - (kSteps - 1) * kGap) / (float)kSteps;
        int   c     = (int)((worldX - wp.x) / (cellW + kGap));
        return (c >= 0 && c < kSteps) ? c : -1;
    }

    void setValueAt(int cell, float worldY) {
        Vec2  wp = worldPos();
        float t  = 1.f - (worldY - wp.y) / height;
        values[cell] = std::clamp(t, 0.f, 1.f);
    }
};

// ── StepSeqNode ───────────────────────────────────────────────────────────────
// Advances one step per Trig input. CV output = step value * Max slider.
// Reset input returns to step 0 without firing outTrig.

class StepSeqNode final : public Node {
public:
    SocketWidget* inTrig  = nullptr;
    SocketWidget* inReset = nullptr;
    SocketWidget* outCV   = nullptr;
    SocketWidget* outTrig = nullptr;

    explicit StepSeqNode(Vec2 pos = {}) : Node("Sequencer", pos) {
        width = 220.f;

        inTrig  = addInput("Trig");
        inReset = addInput("Reset");

        lane = addWidget(std::make_unique<StepLaneWidget>());

        auto* maxSlider = addWidget(std::make_unique<SliderWidget>("Max", 1.f, 2000.f, 440.f));
        maxSlider->onChange = [this](float v) { maxVal = v; };

        outCV   = addOutput("CV");
        outTrig = addOutput("Trig");
    }

    void tick(float /*dt*/) override {
        if (inReset->value.trig)
            step = 0;

        if (inTrig->value.trig) {
            step = (step + 1) % StepLaneWidget::kSteps;
            outTrig->value.trig = lane->values[step] > 0.01f;
        }

        lane->activeStep = step;
        outCV->value.f   = lane->values[step] * maxVal;
    }

private:
    StepLaneWidget* lane   = nullptr;
    int             step   = 0;
    float           maxVal = 440.f;
};
