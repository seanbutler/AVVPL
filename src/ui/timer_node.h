#pragma once
#include "node.h"
#include "socket.h"
#include <cmath>
#include <functional>

// ── ProgressRingWidget ────────────────────────────────────────────────────────
// Read-only circular indicator. getPhase() should return [0, 1].
class ProgressRingWidget final : public Widget {
public:
    std::function<float()> getPhase;
    Color4 arcColor = pal::DialArc;

    static constexpr float kSize = 64.f;
    static constexpr float kR    = 23.f;
    static constexpr float kArcW =  5.f;

    explicit ProgressRingWidget(std::function<float()> fn)
        : getPhase(std::move(fn))
    { width = height = kSize; }

    bool contains(Vec2) const override { return false; }
    void onDrag(Vec2)         override {}

    void draw(tvg::Scene& scene) override {
        Vec2 wp = worldPos();
        float cx = wp.x + kSize * 0.5f;
        float cy = wp.y + kSize * 0.5f;
        float t  = getPhase ? clamp01(getPhase()) : 0.f;

        // Background ring (full circle via appendCircle, transparent fill)
        {
            auto sh = tvg::Shape::gen();
            sh->appendCircle(cx, cy, kR, kR);
            sh->fill(0, 0, 0, 0);
            sh->stroke(kArcW + 2.f);
            sh->stroke(pal::DialTrack.r, pal::DialTrack.g, pal::DialTrack.b, pal::DialTrack.a);
            scene.push(std::move(sh));
        }

        // Progress arc: -90° (12 o'clock) sweeping clockwise by t*360°
        if (t > 0.001f)
            pushArc(scene, cx, cy, kR, -90.f, -90.f + t * 359.9f, arcColor, kArcW);

        // Centre pip
        pushCircle(scene, cx, cy, 4.f, {70, 70, 88, 255});
    }
};

// ── TimerNode ─────────────────────────────────────────────────────────────────
// One-shot countdown. Fires "Done" trigger when it reaches zero.
// Restart by sending a trigger to "Reset", or it auto-arms on construction.
class TimerNode final : public Node {
public:
    // Public so callers can wire them and set initial values.
    SocketWidget* inDuration = nullptr;
    SocketWidget* inReset    = nullptr;
    SocketWidget* outDone    = nullptr;
    SocketWidget* outTime    = nullptr;

    explicit TimerNode(Vec2 pos = {}) : Node("Timer", pos) {
        inDuration = addInput("Duration");
        inReset    = addInput("Reset");

        inDuration->value.f = 2.f;
        remaining            = inDuration->value.f;  // for the ring until the first tick arms it

        ring = addWidget(std::make_unique<ProgressRingWidget>([this] {
            float dur = std::max(inDuration->value.f, 0.001f);
            return remaining / dur;   // 1 = full → 0 = elapsed
        }));
        ring->arcColor = {200, 80, 70, 255};

        outDone = addOutput("Done");
        outTime = addOutput("Time");
    }

    void tick(float dt) override {
        float dur = std::max(inDuration->value.f, 0.001f);

        if (!armed || inReset->value.trig) {
            remaining = dur;
            fired     = false;
            armed     = true;
        }

        outDone->value.trig = false;

        if (!fired && remaining > 0.f) {
            remaining -= dt;
            if (remaining <= 0.f) {
                remaining           = 0.f;
                outDone->value.trig = true;
                fired               = true;
            }
        }

        outTime->value.f = remaining;
    }

private:
    ProgressRingWidget* ring      = nullptr;
    float               remaining = 2.f;
    bool                fired     = false;
    bool                armed     = false;  // countdown loaded from Duration on first tick
};

// ── LoopTimerNode ─────────────────────────────────────────────────────────────
// Fires "Tick" trigger every Interval seconds, forever.
// "Phase" output tracks [0, 1] progress within the current interval.
// "Count" counts how many times it has fired.
class LoopTimerNode final : public Node {
public:
    SocketWidget* inInterval = nullptr;
    SocketWidget* inReset    = nullptr;
    SocketWidget* outTick    = nullptr;
    SocketWidget* outPhase   = nullptr;
    SocketWidget* outCount   = nullptr;

    explicit LoopTimerNode(Vec2 pos = {}) : Node("Loop", pos) {
        inInterval = addInput("Interval");
        inReset    = addInput("Reset");

        inInterval->value.f = 1.f;

        ring = addWidget(std::make_unique<ProgressRingWidget>([this] {
            return phase;
        }));
        ring->arcColor = pal::DialArc;

        outTick  = addOutput("Tick");
        outPhase = addOutput("Phase");
        outCount = addOutput("Count");
    }

    void tick(float dt) override {
        float interval = std::max(inInterval->value.f, 0.001f);

        if (inReset->value.trig) {
            phase = 0.f;
            count = 0;
        }

        outTick->value.trig = false;

        phase += dt / interval;
        if (phase >= 1.f) {
            phase -= std::floor(phase);
            ++count;
            outTick->value.trig = true;
        }

        outPhase->value.f = phase;
        outCount->value.f = (float)count;
    }

private:
    ProgressRingWidget* ring  = nullptr;
    float               phase = 0.f;
    int                 count = 0;
};
