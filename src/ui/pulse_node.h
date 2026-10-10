#pragma once
#include "node.h"
#include "socket.h"
#include "dial.h"
#include "timer_node.h"

// PulseNode — converts a one-shot trigger into a timed gate.
// On each incoming Tick, opens the gate and starts a countdown.
// After Duration seconds, fires a Release trigger and closes the gate.
// Wire: Loop Tick → Pulse In, Pulse Gate → Envelope Gate, Pulse Rel → Envelope Rel.
class PulseNode final : public Node {
public:
    SocketWidget* inTick   = nullptr;
    SocketWidget* outGate  = nullptr;  // trigger: fires when gate opens
    SocketWidget* outRel   = nullptr;  // trigger: fires when gate closes

    explicit PulseNode(Vec2 pos = {}) : Node("Pulse", pos) {
        inTick = addInput("Tick", SocketType::Trigger);

        auto* d = addWidget(std::make_unique<DialWidget>("Dur", 0.01f, 4.f, 0.25f));
        d->onChange = [this](float v){ duration = v; };

        addWidget(std::make_unique<ProgressRingWidget>([this]{
            return remaining > 0.f ? remaining / std::max(duration, 0.001f) : 0.f;
        }));

        outGate = addOutput("Gate", SocketType::Trigger);
        outRel  = addOutput("Rel",  SocketType::Trigger);
    }

    void tick(float dt) override {
        outGate->value.trig = false;
        outRel->value.trig  = false;

        if (inTick->value.trig) {
            remaining = duration;
            outGate->value.trig = true;
        }

        if (remaining > 0.f) {
            remaining -= dt;
            if (remaining <= 0.f) {
                remaining = 0.f;
                outRel->value.trig = true;
            }
        }
    }

private:
    float duration  = 0.25f;
    float remaining = 0.f;
};
