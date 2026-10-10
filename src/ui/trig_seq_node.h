#pragma once
#include "node.h"
#include "socket.h"
#include <array>

// ── GateLaneWidget ────────────────────────────────────────────────────────────
// 8-cell step editor. Each cell is a toggle; click to flip on/off.

class GateLaneWidget final : public Widget {
public:
    static constexpr int   kSteps = 16;
    static constexpr float kH     = 40.f;
    static constexpr float kGap   =  2.f;

    std::array<bool, kSteps> gates     = {};
    int                      activeStep = 0;

    GateLaneWidget() { height = kH; }

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
            bool  on  = gates[i];

            Color4 bg;
            if (on && act)       bg = {120, 210, 120, 255};
            else if (on)         bg = { 70, 160,  70, 220};
            else if (act)        bg = { 45,  58,  80, 255};
            else                 bg = { 28,  28,  40, 255};

            pushRect(scene, cx, wp.y, cellW, height, 2.f, bg);

            if (act)
                pushCircle(scene, cx + cellW * 0.5f, wp.y + 5.f, 2.5f,
                           {200, 230, 255, 255});
        }
    }

    void onMouseDown(Vec2 pt, int) override {
        int c = cellAt(pt.x);
        if (c >= 0) gates[c] = !gates[c];
    }

    nlohmann::json toJson() const override {
        nlohmann::json j = nlohmann::json::array();
        for (bool v : gates) j.push_back(v);
        return j;
    }

    void fromJson(const nlohmann::json& j) override {
        if (!j.is_array()) return;
        for (int i = 0; i < kSteps && i < (int)j.size(); ++i)
            if (j[i].is_boolean())
                gates[i] = j[i].get<bool>();
    }

private:
    int cellAt(float worldX) const {
        Vec2  wp    = worldPos();
        float cellW = (width - (kSteps - 1) * kGap) / (float)kSteps;
        int   c     = (int)((worldX - wp.x) / (cellW + kGap));
        return (c >= 0 && c < kSteps) ? c : -1;
    }
};

// ── TrigSeqNode ───────────────────────────────────────────────────────────────
// Advances one step per Trig input. Fires outTrig only on steps that are on.
// Reset input rewinds so the next Trig plays step 0; it does not fire outTrig.

class TrigSeqNode final : public Node {
public:
    SocketWidget* inTrig  = nullptr;
    SocketWidget* inReset = nullptr;
    SocketWidget* outTrig = nullptr;

    explicit TrigSeqNode(Vec2 pos = {}) : Node("Sequencer Trig", pos) {
        width = 220.f;

        inTrig  = addInput("Trig");
        inReset = addInput("Reset");

        lane = addWidget(std::make_unique<GateLaneWidget>());

        outTrig = addOutput("Trig");
    }

    void tick(float /*dt*/) override {
        if (inReset->value.trig) {
            step    = 0;
            atStart = true;
        }

        if (inTrig->value.trig) {
            if (atStart) atStart = false;            // first Trig plays step 0
            else         step = (step + 1) % GateLaneWidget::kSteps;
            outTrig->value.trig = lane->gates[step];
        }

        lane->activeStep = step;
    }

private:
    GateLaneWidget* lane = nullptr;
    int             step = 0;
    bool atStart = true;  // next Trig plays step 0 rather than advancing
};
