#pragma once
#include "widget.h"
#include <functional>
#include <string>

enum class SocketDir { Input, Output };

class SocketWidget final : public Widget {
public:
    SocketDir   dir;
    int         index = 0;
    std::string label;
    std::function<void()> onConnect;

    static constexpr float kR = 7.f;

    Value value;   // current value on this socket (written by owning node, read by downstream)

    SocketWidget(SocketDir d, int idx, std::string lbl)
        : dir(d), index(idx), label(std::move(lbl))
    { width = height = kR * 2.f; }

    Vec2 worldCenter() const {
        Vec2 wp = worldPos();
        return {wp.x + kR, wp.y + kR};
    }

    bool contains(Vec2 pt) const override {
        Vec2 c = worldCenter();
        float dx = pt.x - c.x, dy = pt.y - c.y;
        return dx*dx + dy*dy <= (kR+3.f)*(kR+3.f);  // slightly enlarged for easier picking
    }

    nlohmann::json toJson()  const override { return value.f; }
    void fromJson(const nlohmann::json& j) override {
        if (j.is_number()) value.f = j.get<float>();
    }

    void draw(tvg::Scene& scene) override {
        Vec2 c = worldCenter();
        Color4 fill = (dir == SocketDir::Input) ? pal::SockIn : pal::SockOut;
        pushCircle(scene, c.x, c.y, kR,          fill,          pal::NodeBorder, 1.5f);
        pushCircle(scene, c.x, c.y, kR * 0.35f,  {255,255,255,180});
    }

    // Sockets don't move with onDrag; they are laid out by their parent node.
    void onDrag(Vec2) override {}
};
