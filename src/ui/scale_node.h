#pragma once
#include "node.h"
#include "socket.h"
#include "slider.h"
#include <algorithm>

// ScaleNode — maps an input float from one range to another.
// out = outMin + (in - inMin) / (inMax - inMin) * (outMax - outMin)
// Clamps input to [inMin, inMax] before mapping.
class ScaleNode final : public Node {
public:
    SocketWidget* inValue  = nullptr;
    SocketWidget* outValue = nullptr;

    explicit ScaleNode(Vec2 pos = {}) : Node("Scale", pos) {
        inValue = addInput("In");

        auto* sInMin  = addWidget(std::make_unique<SliderWidget>("In Min",  -2000.f, 2000.f,    0.f));
        auto* sInMax  = addWidget(std::make_unique<SliderWidget>("In Max",  -2000.f, 2000.f, 2000.f));
        auto* sOutMin = addWidget(std::make_unique<SliderWidget>("Out Min", -2000.f, 2000.f,    0.f));
        auto* sOutMax = addWidget(std::make_unique<SliderWidget>("Out Max", -2000.f, 2000.f,    1.f));

        sInMin ->onChange = [this](float v){ inMin  = v; };
        sInMax ->onChange = [this](float v){ inMax  = v; };
        sOutMin->onChange = [this](float v){ outMin = v; };
        sOutMax->onChange = [this](float v){ outMax = v; };

        outValue = addOutput("Out");
    }

    void tick(float /*dt*/) override {
        float span = inMax - inMin;
        float t = (std::abs(span) < 1e-6f) ? 0.f
                : (inValue->value.f - inMin) / span;
        t = std::clamp(t, 0.f, 1.f);
        outValue->value.f = outMin + t * (outMax - outMin);
    }

    nlohmann::json toJson() const override {
        return { {"inMin", inMin}, {"inMax", inMax},
                 {"outMin", outMin}, {"outMax", outMax} };
    }

    void fromJson(const nlohmann::json& j) override {
        if (j.contains("inMin"))  inMin  = j["inMin"];
        if (j.contains("inMax"))  inMax  = j["inMax"];
        if (j.contains("outMin")) outMin = j["outMin"];
        if (j.contains("outMax")) outMax = j["outMax"];
    }

private:
    float inMin  =    0.f;
    float inMax  = 2000.f;
    float outMin =    0.f;
    float outMax =    1.f;
};
