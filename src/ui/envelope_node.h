#pragma once
#include "node.h"
#include "socket.h"
#include "dial.h"
#include "timer_node.h"

// EnvelopeNode — ADSR envelope triggered by a one-shot Gate trigger.
// outCV outputs a 0-1 float each tick; wire it to a ToneNode Amp CV input.
// With sustain=0 (default) the shape is attack → decay → idle, like a piano.
class EnvelopeNode final : public Node {
public:
    SocketWidget* inGate = nullptr;
    SocketWidget* outCV  = nullptr;

    explicit EnvelopeNode(Vec2 pos = {}) : Node("Envelope", pos) {
        inGate = addInput("Gate");

        auto* aD = addWidget(std::make_unique<DialWidget>("Atk", 0.001f, 1.f,  0.01f));
        auto* dD = addWidget(std::make_unique<DialWidget>("Dec", 0.01f,  2.f,  0.3f));
        auto* sD = addWidget(std::make_unique<DialWidget>("Sus", 0.f,    1.f,  0.f));
        auto* rD = addWidget(std::make_unique<DialWidget>("Rel", 0.01f,  1.f,  0.1f));

        aD->onChange = [this](float v){ attack  = v; };
        dD->onChange = [this](float v){ decay   = v; };
        sD->onChange = [this](float v){ sustain = v; };
        rD->onChange = [this](float v){ release = v; };

        auto* ring = addWidget(std::make_unique<ProgressRingWidget>([this]{ return level; }));
        ring->arcColor = pal::SliderFill;

        outCV = addOutput("CV Out");
    }

    void tick(float dt) override {
        if (inGate->value.trig) {
            envPhase = Phase::Attack;
            level = 0.f;
        }

        switch (envPhase) {
            case Phase::Attack:
                level += dt / std::max(attack, 0.001f);
                if (level >= 1.f) { level = 1.f; envPhase = Phase::Decay; }
                break;
            case Phase::Decay:
                level -= dt / std::max(decay, 0.001f) * (1.f - sustain);
                if (level <= sustain) { level = sustain; envPhase = Phase::Sustain; }
                break;
            case Phase::Sustain:
                level = sustain;
                if (sustain <= 0.f) envPhase = Phase::Idle;
                break;
            case Phase::Release:
                level -= dt / std::max(release, 0.001f);
                if (level <= 0.f) { level = 0.f; envPhase = Phase::Idle; }
                break;
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
    float decay   = 0.3f;
    float sustain = 0.f;
    float release = 0.1f;
};
