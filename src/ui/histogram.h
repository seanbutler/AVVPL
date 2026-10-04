#pragma once
#include "widget.h"
#include <functional>
#include <vector>

class HistogramWidget final : public Widget {
public:
    std::vector<float> values;   // each value in [0, 1]
    std::function<void(int, float)> onChange;

    static constexpr float kH       = 60.f;
    static constexpr float kBarGap  =  1.f;

    explicit HistogramWidget(int bins, float initial = 0.5f) {
        values.assign(bins, initial);
        height = kH;
    }

    bool contains(Vec2 pt) const override {
        Vec2 wp = worldPos();
        return pt.x >= wp.x && pt.x <= wp.x + width
            && pt.y >= wp.y && pt.y <= wp.y + kH;
    }

    void draw(tvg::Scene& scene) override;

    nlohmann::json toJson() const override {
        return nlohmann::json(values);
    }
    void fromJson(const nlohmann::json& j) override {
        if (!j.is_array()) return;
        for (size_t i = 0; i < values.size() && i < j.size(); ++i)
            values[i] = clamp01(j[i].get<float>());
    }

    void onMouseDown(Vec2 pt, int) override { setBarFromPt(pt); }
    void onMouseMove(Vec2 pt, Vec2) override { if (activeBin >= 0) setBarFromPt(pt); }
    void onMouseUp  (Vec2, int) override { activeBin = -1; }
    void onDrag(Vec2) override {}

private:
    int activeBin = -1;

    int binAt(float worldX) const {
        Vec2 wp = worldPos();
        int n = (int)values.size();
        float bw = width / n;
        int b = (int)((worldX - wp.x) / bw);
        return (b >= 0 && b < n) ? b : -1;
    }

    void setBarFromPt(Vec2 worldPt) {
        int b = binAt(worldPt.x);
        if (b < 0) return;
        activeBin = b;
        Vec2 wp = worldPos();
        float t = 1.f - (worldPt.y - wp.y) / kH;
        float v = clamp01(t);
        if (v != values[b]) {
            values[b] = v;
            if (onChange) onChange(b, v);
        }
    }
};
