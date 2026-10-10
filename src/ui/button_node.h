#pragma once
#include "node.h"
#include "socket.h"
#include "node_canvas.h"

// ── ButtonPadWidget ───────────────────────────────────────────────────────────
// A clickable pad. Records presses for the owning node to pick up on its next
// tick; stays lit while held (and briefly after a quick click).

class ButtonPadWidget final : public Widget {
public:
    static constexpr float kH = 40.f;

    bool  held     = false;  // mouse currently down on the pad
    bool  pressed  = false;  // set on mouse down; cleared by the node once consumed
    bool  released = false;  // set on mouse up; cleared by the node once consumed
    float flash    = 0.f;    // seconds of highlight remaining after a press

    ButtonPadWidget() { height = kH; }

    bool contains(Vec2 pt) const override {
        Vec2 wp = worldPos();
        return pt.x >= wp.x && pt.x <= wp.x + width
            && pt.y >= wp.y && pt.y <= wp.y + height;
    }

    void onMouseDown(Vec2, int) override { held = true; pressed = true; }
    void onMouseUp  (Vec2, int) override { held = false; released = true; }
    void onDrag(Vec2) override {}

    void draw(tvg::Scene& scene) override {
        Vec2 wp  = worldPos();
        bool lit = held || flash > 0.f;
        pushRect(scene, wp.x, wp.y, width, height, 6.f,
                 lit ? Color4{200, 90, 70, 255} : Color4{60, 40, 44, 255},
                 pal::NodeBorder, 1.5f);

        if (!NodeCanvas::fontLoaded()) return;
        auto t = tvg::Text::gen();
        if (t->font(NodeCanvas::fontName().c_str(), 11.f) != tvg::Result::Success) return;
        static constexpr const char* kLabel = "PRESS";
        t->text(kLabel);
        t->fill(lit ? 255 : 200, lit ? 240 : 180, lit ? 235 : 185);
        float textW = 5 * 7.f;  // approx width of "PRESS" at 11pt
        t->translate(wp.x + (width - textW) * 0.5f, wp.y + kH * 0.5f - 7.f);
        scene.push(std::move(t));
    }
};

// ── ButtonNode ────────────────────────────────────────────────────────────────
// Manual trigger. Trig fires once on press, Rel once on release (same
// convention as Pulse, so Trig/Rel can drive Envelope Gate/Rel directly).
// Wire Trig into the Reset inputs of Tempo / sequencers to restart a tune.

class ButtonNode final : public Node {
public:
    SocketWidget* outTrig = nullptr;
    SocketWidget* outRel  = nullptr;

    static constexpr float kFlashTime = 0.12f;
    static constexpr float kLabelRoom = 36.f;

    explicit ButtonNode(Vec2 pos = {}) : Node("Button", pos) {
        width = 140.f;
        pad = addWidget(std::make_unique<ButtonPadWidget>());
        pad->width -= kLabelRoom;  // leave space for the output socket labels

        outTrig = addOutput("Trig");
        outRel  = addOutput("Rel");
    }

    void tick(float dt) override {
        outTrig->value.trig = false;
        outRel->value.trig  = false;
        if (pad->pressed) {
            pad->pressed = false;
            pad->flash   = kFlashTime;
            outTrig->value.trig = true;
        }
        if (pad->released) {
            pad->released = false;
            outRel->value.trig = true;
        }
        pad->flash = std::max(0.f, pad->flash - dt);
    }

private:
    ButtonPadWidget* pad = nullptr;
};
