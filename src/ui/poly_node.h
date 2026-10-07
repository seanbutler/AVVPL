#pragma once
#include "node.h"
#include "socket.h"
#include <thorvg.h>
#include <cmath>
#include <algorithm>

static constexpr float kTau = 6.28318530718f;

// ── PolyDisplayWidget ─────────────────────────────────────────────────────────
class PolyDisplayWidget final : public Widget {
public:
    int   sides    = 6;
    float radius   = 50.f;
    float rotation = 0.f;   // degrees

    static constexpr float kSize = 120.f;

    PolyDisplayWidget() { width = height = kSize; }

    bool contains(Vec2) const override { return false; }
    void onDrag(Vec2)         override {}

    void draw(tvg::Scene& scene) override {
        Vec2 wp = worldPos();
        float cx = wp.x + kSize * 0.5f;
        float cy = wp.y + kSize * 0.5f;

        // Background
        pushRect(scene, wp.x, wp.y, kSize, kSize, 6.f, {30, 30, 40, 255});

        int n = std::clamp(sides, 3, 64);
        float r = std::clamp(radius, 0.f, kSize * 0.5f - 2.f);
        float rotRad = rotation * (kTau / 360.f);

        auto sh = tvg::Shape::gen();
        for (int i = 0; i <= n; ++i) {
            float angle = rotRad + (i % n) * kTau / n;
            float x = cx + r * std::cos(angle);
            float y = cy + r * std::sin(angle);
            if (i == 0) sh->moveTo(x, y);
            else        sh->lineTo(x, y);
        }
        sh->close();
        sh->fill(80, 160, 220, 60);
        sh->stroke(2.f);
        sh->stroke(140, 200, 255, 255);
        scene.push(std::move(sh));

        // Centre dot
        pushCircle(scene, cx, cy, 2.f, {140, 200, 255, 180});
    }
};

// ── PolyNode ──────────────────────────────────────────────────────────────────
class PolyNode final : public Node {
public:
    SocketWidget* inSides    = nullptr;
    SocketWidget* inRadius   = nullptr;
    SocketWidget* inRotation = nullptr;

    explicit PolyNode(Vec2 pos = {}) : Node("Polygon", pos) {
        inSides    = addInput("Sides");
        inRadius   = addInput("Radius");
        inRotation = addInput("Rotation");

        inSides->value.f    = 6.f;
        inRadius->value.f   = 50.f;
        inRotation->value.f = 0.f;

        display = addWidget(std::make_unique<PolyDisplayWidget>());
    }

    void tick(float /*dt*/) override {
        display->sides    = std::clamp((int)std::round(inSides->value.f),    3, 64);
        display->radius   = std::clamp(inRadius->value.f,   0.f, 58.f);
        display->rotation = inRotation->value.f;
    }

    nlohmann::json toJson() const override {
        return { {"sides",    inSides->value.f},
                 {"radius",   inRadius->value.f},
                 {"rotation", inRotation->value.f} };
    }

    void fromJson(const nlohmann::json& j) override {
        if (j.contains("sides"))    inSides->value.f    = j["sides"];
        if (j.contains("radius"))   inRadius->value.f   = j["radius"];
        if (j.contains("rotation")) inRotation->value.f = j["rotation"];
    }

private:
    PolyDisplayWidget* display = nullptr;
};
