#pragma once
#include "node.h"
#include "socket.h"
#include "slider.h"
#include "dial.h"
#include "timer_node.h"

// TempoNode — master clock at a set BPM with swing.
// Outputs a Tick at eighth-note rate. Feed into Clock Div for subdivisions:
//   /1 = eighth note, /2 = quarter, /4 = half, /8 = whole.
//
// Swing delays every other (upbeat) eighth note, giving a shuffle feel.
// Swing=0 is straight; Swing≈0.17 is triplet (jazz) swing; Swing=0.45 is
// heavily dotted.
class TempoNode final : public Node {
public:
    SocketWidget* inReset = nullptr;
    SocketWidget* outTick = nullptr;
    SocketWidget* outBeat = nullptr;  // fires every quarter note (every 2 ticks)

    explicit TempoNode(Vec2 pos = {}) : Node("Tempo", pos) {
        inReset = addInput("Reset");

        auto* bpmSlider = addWidget(std::make_unique<SliderWidget>("BPM", 40.f, 240.f, 120.f));
        bpmSlider->onChange = [this](float v){ bpm = v; };

        auto* swingDial = addWidget(std::make_unique<DialWidget>("Swing", 0.f, 0.45f, 0.f));
        swingDial->onChange = [this](float v){ swing = v; };

        addWidget(std::make_unique<ProgressRingWidget>([this]{
            return elapsed / std::max(currentInterval(), 0.001f);
        }));

        outTick = addOutput("Tick");
        outBeat = addOutput("Beat");
    }

    void tick(float dt) override {
        outTick->value.trig = false;
        outBeat->value.trig = false;

        if (inReset->value.trig) {
            elapsed  = 0.f;
            evenStep = true;
            beatCount = 0;
        }

        elapsed += dt;
        if (elapsed >= currentInterval()) {
            elapsed -= currentInterval();
            evenStep = !evenStep;
            outTick->value.trig = true;
            ++beatCount;
            if (beatCount % 2 == 0)
                outBeat->value.trig = true;
        }
    }

private:
    float bpm      = 120.f;
    float swing    = 0.f;
    float elapsed  = 0.f;
    bool  evenStep = true;   // true = downbeat eighth, false = upbeat eighth
    int   beatCount = 0;

    // Downbeat eighths are longer with swing; upbeat eighths are shorter.
    float currentInterval() const {
        float eighth = 30.f / std::max(bpm, 1.f);  // 60/BPM / 2
        return evenStep ? eighth * (1.f + swing)
                        : eighth * (1.f - swing);
    }
};
