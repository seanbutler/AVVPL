#pragma once
#include "node.h"
#include "socket.h"
#include "dial.h"
#include "timer_node.h"
#include <cmath>

// LfoNode — low-frequency oscillator; outputs a 0–1 CV value cycling at Rate Hz.
// Waveform type is read from an inWave CV input (0=Sine 1=Sqr 2=Saw 3=Tri 4=Noise),
// matching the WaveformNode output — wire one WaveformNode into both Tone and LFO.
// Out CV feeds Amp, filter cutoff, pitch (via Scale), or any other CV target.
// Trig Out fires once per cycle — useful for syncing other nodes to the LFO rate.
class LfoNode final : public Node {
public:
    SocketWidget* inWave  = nullptr;
    SocketWidget* inReset = nullptr;
    SocketWidget* outCV   = nullptr;
    SocketWidget* outTrig = nullptr;

    explicit LfoNode(Vec2 pos = {}) : Node("LFO", pos) {
        inWave  = addInput("Wave",  SocketType::CV);
        inReset = addInput("Reset", SocketType::Trigger);

        // Rate dial centred in the node body.
        static constexpr float kDialOffX = (kMinWidth - DialWidget::kSize) * 0.5f;
        float y = height;
        auto d = std::make_unique<DialWidget>("Rate", 0.01f, 20.f, 1.f);
        d->localPos  = {kDialOffX, y};
        d->logScale  = true;
        d->onChange  = [this](float v){ rate = v; };
        addChild(std::move(d));
        height = y + DialWidget::kSize + kPadY;

        auto* rw = addWidget(std::make_unique<ProgressRingWidget>([this]{
            return outCV ? outCV->value.f : 0.f;
        }));
        rw->arcColor = pal::DialArc;

        outCV   = addOutput("Out",  SocketType::CV);
        outTrig = addOutput("Trig", SocketType::Trigger);
    }

    void tick(float dt) override {
        if (inReset->value.trig) phase = 0.0;

        phase += rate * dt;
        bool wrapped = false;
        if (phase >= 1.0) { phase -= std::floor(phase); wrapped = true; }

        int   w = inWave->connected ? (int)inWave->value.f : 0;
        float v = 0.f;
        switch (w) {
            default:
            case 0: v = 0.5f + 0.5f * std::sin(2.f * kPi * (float)phase); break;
            case 1: v = (float)phase < 0.5f ? 1.f : 0.f; break;
            case 2: v = (float)phase; break;
            case 3: v = (float)phase < 0.5f ? 2.f*(float)phase : 2.f*(1.f-(float)phase); break;
            case 4: { // noise: random value held for each cycle
                v = noiseVal;
                if (wrapped) noiseVal = (float)rand() / (float)RAND_MAX;
                break;
            }
        }

        outCV->value.f      = v;
        outTrig->value.trig = wrapped;
    }

    nlohmann::json toJson() const override {
        auto j = Node::toJson();
        j["rate"] = rate;
        return j;
    }

    void fromJson(const nlohmann::json& j) override {
        Node::fromJson(j);
        if (j.contains("rate")) rate = j["rate"].get<float>();
        phase = 0.0;
    }

private:
    float  rate     = 1.f;
    double phase    = 0.0;
    float  noiseVal = 0.f;
};
