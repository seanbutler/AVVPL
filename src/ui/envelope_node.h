#pragma once
#include "node.h"
#include "socket.h"
#include "dial.h"
#include "timer_node.h"
#include <cmath>

// EnvelopeNode — ADSR envelope triggered by a one-shot Gate trigger.
// Decay and Release use exponential curves for a natural instrument feel.
// With sustain=0 the shape is attack → exponential decay → idle (piano/cymbal).
class EnvelopeNode final : public Node {
public:
    SocketWidget* inGate    = nullptr;
    SocketWidget* inRelease = nullptr;
    SocketWidget* outCV     = nullptr;

    explicit EnvelopeNode(Vec2 pos = {}) : Node("Envelope", pos) {
        inGate    = addInput("Gate", SocketType::Trigger);
        inRelease = addInput("Rel",  SocketType::Trigger);


        // 2×2 dial grid: two 80px columns inside the 160px body.
        // Each 60px dial is centred in its column.
        static constexpr float kColW     = (kMinWidth - 2.f * kPadX) * 0.5f;
        static constexpr float kDialOffX = (kColW - DialWidget::kSize) * 0.5f;
        float x1 = kPadX + kDialOffX;
        float x2 = kPadX + kColW + kDialOffX;
        float y1 = height;
        float y2 = y1 + DialWidget::kSize + kPadY;

        auto place = [this](std::string lbl, float mn, float mx, float v, float x, float y) {
            auto d = std::make_unique<DialWidget>(std::move(lbl), mn, mx, v);
            d->localPos = {x, y};
            auto* raw = d.get();
            addChild(std::move(d));
            return raw;
        };

        auto* aD = place("Atk", 0.001f, 2.f,  0.01f, x1, y1);
        auto* dD = place("Dec", 0.01f,  8.f,  0.5f,  x2, y1);
        auto* sD = place("Sus", 0.f,    1.f,  0.f,   x1, y2);
        auto* rD = place("Rel", 0.01f,  8.f,  0.3f,  x2, y2);

        height = y2 + DialWidget::kSize + kPadY;

        aD->onChange = [this](float v){ attack  = v; };
        dD->onChange = [this](float v){ decay   = v; };
        sD->onChange = [this](float v){ sustain = v; };
        rD->onChange = [this](float v){ release = v; };

        // Ring centred below the grid
        auto rw = std::make_unique<ProgressRingWidget>([this]{ return level; });
        rw->arcColor = pal::SliderFill;
        rw->localPos = {(kMinWidth - ProgressRingWidget::kSize) * 0.5f, height};
        height      += ProgressRingWidget::kSize + kPadY;
        addChild(std::move(rw));

        outCV = addOutput("CV Out", SocketType::CV);
    }

    void tick(float dt) override {
        if (inGate->value.trig) {
            envPhase = Phase::Attack;
        }
        if (inRelease->value.trig && envPhase != Phase::Idle) {
            envPhase = Phase::Release;
        }

        switch (envPhase) {
            case Phase::Attack:
                level += dt / std::max(attack, 0.001f);
                if (level >= 1.f) { level = 1.f; envPhase = Phase::Decay; }
                break;

            case Phase::Decay: {
                // Exponential decay: time constant = decay / 3 so level reaches
                // ~5% of (peak - sustain) after one full decay period.
                float tau = std::max(decay, 0.001f) / 3.f;
                level = sustain + (level - sustain) * std::exp(-dt / tau);
                if (level <= sustain + 0.001f) {
                    level = sustain;
                    envPhase = (sustain > 0.f) ? Phase::Sustain : Phase::Idle;
                }
                break;
            }

            case Phase::Sustain:
                level = sustain;
                break;

            case Phase::Release: {
                float tau = std::max(release, 0.001f) / 3.f;
                level *= std::exp(-dt / tau);
                if (level < 0.001f) { level = 0.f; envPhase = Phase::Idle; }
                break;
            }

            case Phase::Idle:
                break;
        }

        outCV->value.f = level;
    }

private:
    enum class Phase { Idle, Attack, Decay, Sustain, Release };
    Phase envPhase = Phase::Idle;
    float level   = 0.f;
    float attack  = 0.01f;
    float decay   = 0.5f;
    float sustain = 0.f;
    float release = 0.3f;
};
