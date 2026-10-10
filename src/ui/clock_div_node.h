#pragma once
#include "node.h"
#include "socket.h"

// ClockDivNode — takes a master clock trigger and outputs subdivisions.
// /1 fires every tick, /2 every 2nd, /4 every 4th, /8 every 8th.
// Reset input returns all counters to zero on the next tick.
class ClockDivNode final : public Node {
public:
    SocketWidget* inTick  = nullptr;
    SocketWidget* inReset = nullptr;
    SocketWidget* outDiv1 = nullptr;
    SocketWidget* outDiv2 = nullptr;
    SocketWidget* outDiv4 = nullptr;
    SocketWidget* outDiv8 = nullptr;

    explicit ClockDivNode(Vec2 pos = {}) : Node("Clock Div", pos) {
        inTick  = addInput("Tick",  SocketType::Trigger);
        inReset = addInput("Reset", SocketType::Trigger);

        outDiv1 = addOutput("/1", SocketType::Trigger);
        outDiv2 = addOutput("/2", SocketType::Trigger);
        outDiv4 = addOutput("/4", SocketType::Trigger);
        outDiv8 = addOutput("/8", SocketType::Trigger);
    }

    void tick(float /*dt*/) override {
        outDiv1->value.trig = false;
        outDiv2->value.trig = false;
        outDiv4->value.trig = false;
        outDiv8->value.trig = false;

        if (inReset->value.trig)
            count = 0;

        if (inTick->value.trig) {
            outDiv1->value.trig = true;
            outDiv2->value.trig = (count % 2) == 0;
            outDiv4->value.trig = (count % 4) == 0;
            outDiv8->value.trig = (count % 8) == 0;
            ++count;
        }
    }

private:
    int count = 0;
};
