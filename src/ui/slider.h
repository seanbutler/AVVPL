#pragma once
#include "widget.h"
#include <functional>
#include <string>

class SliderWidget final : public Widget {
public:
    float min, max, value;
    std::string label;
    std::function<void(float)> onChange;

    static constexpr float kH       = 28.f;
    static constexpr float kTrackH  =  5.f;
    static constexpr float kThumbR  =  7.f;
    static constexpr float kPad     =  kThumbR + 2.f;

    SliderWidget(std::string lbl, float mn, float mx, float v)
        : min(mn), max(mx), value(clampf(v, mn, mx)), label(std::move(lbl))
    { height = kH; }

    float norm() const { return (value - min) / (max - min); }

    bool contains(Vec2 pt) const override {
        Vec2 wp = worldPos();
        return pt.x >= wp.x && pt.x <= wp.x + width
            && pt.y >= wp.y && pt.y <= wp.y + kH;
    }

    void draw(tvg::Scene& scene) override;
    void onMouseDown(Vec2 pt, int) override { setFromX(pt.x); }
    void onMouseMove(Vec2 pt, Vec2)  override { setFromX(pt.x); }
    void onDrag(Vec2) override {}

    nlohmann::json toJson()  const override { return value; }
    void fromJson(const nlohmann::json& j) override {
        if (!j.is_number()) return;
        value = clampf(j.get<float>(), min, max);
        if (onChange) onChange(value);  // so the owning node picks up loaded values
    }

    // Returns a "nice" step size scaled to the slider's range.
    float stepSize() const;

private:
    void setFromX(float worldX);
};
