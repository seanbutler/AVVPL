#pragma once
#include "node.h"
#include "socket.h"
#include "slider.h"

// NumberNode — a slider with configurable Min/Max range that outputs a float.
class NumberNode final : public Node {
public:
    SocketWidget* outValue = nullptr;

    explicit NumberNode(Vec2 pos = {}) : Node("Number", pos) {
        auto* sMin = addWidget(std::make_unique<SliderWidget>("Min", -2000.f, 2000.f, 0.f));
        auto* sMax = addWidget(std::make_unique<SliderWidget>("Max", -2000.f, 2000.f, 2000.f));
        val = addWidget(std::make_unique<SliderWidget>("Value", 0.f, 2000.f, 440.f));

        sMin->onChange = [this](float v) {
            val->min   = v;
            val->value = clampf(val->value, val->min, val->max);
            outValue->value.f = val->value;
        };
        sMax->onChange = [this](float v) {
            val->max   = v;
            val->value = clampf(val->value, val->min, val->max);
            outValue->value.f = val->value;
        };
        val->onChange = [this](float v) { outValue->value.f = v; };

        outValue = addOutput("Value");
        outValue->value.f = 440.f;
    }

    void tick(float /*dt*/) override {}

    nlohmann::json toJson() const override {
        return { {"min", val->min}, {"max", val->max}, {"value", val->value} };
    }

    void fromJson(const nlohmann::json& j) override {
        if (j.contains("min"))   val->min   = j["min"];
        if (j.contains("max"))   val->max   = j["max"];
        if (j.contains("value")) val->value = clampf(j["value"].get<float>(), val->min, val->max);
        outValue->value.f = val->value;
    }

private:
    SliderWidget* val = nullptr;
};
