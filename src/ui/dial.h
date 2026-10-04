#pragma once
#include "widget.h"
#include <functional>
#include <string>

class DialWidget final : public Widget {
public:
    float min, max, value;
    std::string label;
    std::function<void(float)> onChange;

    // Screen-CW-from-right degrees. Arc starts at "7 o'clock" (120°) and
    // sweeps clockwise 270° to "4 o'clock" (390° = 30°).
    static constexpr float kStartDeg = 120.f;
    static constexpr float kSweep    = 270.f;
    static constexpr float kSize     = 60.f;   // widget bounding box
    static constexpr float kR        = 22.f;   // arc radius
    static constexpr float kKnobR    = 12.f;
    static constexpr float kArcW     =  4.f;

    DialWidget(std::string lbl, float mn, float mx, float v)
        : min(mn), max(mx), value(clampf(v, mn, mx)), label(std::move(lbl))
    { width = height = kSize; }

    float norm() const { return (value - min) / (max - min); }

    Vec2 center() const {
        Vec2 wp = worldPos();
        return {wp.x + kSize*0.5f, wp.y + kSize*0.5f};
    }

    bool contains(Vec2 pt) const override {
        Vec2 c = center();
        float dx = pt.x - c.x, dy = pt.y - c.y;
        return dx*dx + dy*dy <= kSize*kSize*0.25f;
    }

    void draw(tvg::Scene& scene) override;
    void onMouseDown(Vec2, int) override { dragY = -1.f; }
    void onMouseMove(Vec2 pt, Vec2 delta) override;
    void onDrag(Vec2) override {}

    nlohmann::json toJson()  const override { return value; }
    void fromJson(const nlohmann::json& j) override {
        if (j.is_number()) value = clampf(j.get<float>(), min, max);
    }

private:
    float dragY = -1.f;

    void setNorm(float t) {
        float v = min + clamp01(t) * (max - min);
        if (v != value) { value = v; if (onChange) onChange(v); }
    }
};
