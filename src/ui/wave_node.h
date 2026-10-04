#pragma once
#include "node.h"
#include "socket.h"
#include "slider.h"
#include "selector.h"

// WaveNode — outputs a frequency value and a waveform type index (0–3).
// Downstream ToneNode reads these to drive audio synthesis.
class WaveNode final : public Node {
public:
    SocketWidget* outFreq = nullptr;
    SocketWidget* outWave = nullptr;

    explicit WaveNode(Vec2 pos = {}) : Node("Wave", pos) {
        auto* freqSlider = addWidget(std::make_unique<SliderWidget>("Freq", 20.f, 2000.f, 440.f));
        freqSlider->onChange = [this](float v) { outFreq->value.f = v; };

        auto* sel = addWidget(std::make_unique<SelectorWidget>(
            std::vector<std::string>{"Sine", "Sqr", "Saw", "Tri"}, 0));
        sel->onChange = [this](int i) { outWave->value.f = (float)i; };

        outFreq = addOutput("Freq");
        outWave = addOutput("Wave");

        outFreq->value.f = 440.f;
        outWave->value.f = 0.f;
    }

    void tick(float /*dt*/) override {
        // outFreq and outWave are written by widget callbacks and propagated by WireLayer.
        // Nothing extra needed here.
    }
};
