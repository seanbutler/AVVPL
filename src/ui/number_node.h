#pragma once
#include "node.h"
#include "socket.h"
#include "slider.h"

// NumberNode — a single slider that outputs a float value.
// Use it as a frequency source, a gain value, or any scalar CV.
class NumberNode final : public Node {
public:
    SocketWidget* outValue = nullptr;

    explicit NumberNode(Vec2 pos = {}) : Node("Number", pos) {
        auto* s = addWidget(std::make_unique<SliderWidget>("Value", 0.f, 2000.f, 440.f));
        s->onChange = [this](float v) { outValue->value.f = v; };

        outValue = addOutput("Value");
        outValue->value.f = 440.f;
    }

    void tick(float /*dt*/) override {}
};
