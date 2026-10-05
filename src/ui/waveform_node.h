#pragma once
#include "node.h"
#include "socket.h"
#include "selector.h"

// WaveformNode — outputs a waveform type as a float (0=Sine 1=Sqr 2=Saw 3=Tri).
// Wire its output to a Tone Wave input.
class WaveformNode final : public Node {
public:
    SocketWidget* outWave = nullptr;

    explicit WaveformNode(Vec2 pos = {}) : Node("Waveform", pos) {
        auto* sel = addWidget(std::make_unique<SelectorWidget>(
            std::vector<std::string>{"Sine", "Sqr", "Saw", "Tri"}, 0));
        sel->onChange = [this](int i) { outWave->value.f = (float)i; };

        outWave = addOutput("Wave");
        outWave->value.f = 0.f;
    }

    void tick(float /*dt*/) override {}
};
