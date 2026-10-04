#pragma once
#include "types.h"
#include <thorvg.h>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>

class Widget {
public:
    Vec2    localPos;
    float   width  = 0.f;
    float   height = 0.f;
    Widget* parent = nullptr;
    bool    visible = true;

    std::vector<std::unique_ptr<Widget>> children;

    Vec2 worldPos() const {
        if (parent) return parent->worldPos() + localPos;
        return localPos;
    }

    // Deepest child first; returns nullptr if no hit.
    virtual Widget* hitTest(Vec2 worldPt) {
        if (!visible) return nullptr;
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            if (auto* h = (*it)->hitTest(worldPt)) return h;
        }
        return contains(worldPt) ? this : nullptr;
    }

    void drawAll(tvg::Scene& scene) {
        if (!visible) return;
        draw(scene);
        for (auto& c : children) c->drawAll(scene);
    }

    virtual void draw(tvg::Scene& scene) = 0;
    virtual bool contains(Vec2 worldPt) const = 0;

    virtual void onMouseDown(Vec2 /*pt*/, int /*btn*/) {}
    virtual void onMouseUp  (Vec2 /*pt*/, int /*btn*/) {}
    virtual void onMouseMove(Vec2 /*pt*/, Vec2 /*delta*/) {}

    // Default drag: translate self in parent space.
    virtual void onDrag(Vec2 delta) { localPos += delta; }

    // Serialization: concrete value widgets override these.
    virtual nlohmann::json  toJson()   const    { return nullptr; }
    virtual void            fromJson(const nlohmann::json&) {}

    void addChild(std::unique_ptr<Widget> w) {
        w->parent = this;
        children.push_back(std::move(w));
    }

    virtual ~Widget() = default;

protected:
    static void pushRect  (tvg::Scene&, float x, float y, float w, float h,
                            float rx, Color4 fill, Color4 stroke={0,0,0,0}, float sw=0.f);
    static void pushCircle(tvg::Scene&, float cx, float cy, float r,
                            Color4 fill, Color4 stroke={0,0,0,0}, float sw=0.f);
    static void pushLine  (tvg::Scene&, float x1, float y1, float x2, float y2,
                            Color4 c, float sw=1.5f);
    // Manual arc: startDeg and endDeg in screen-CW-from-right degrees; Y-down convention.
    static void pushArc   (tvg::Scene&, float cx, float cy, float r,
                            float startDeg, float endDeg, Color4 c, float sw, int segs=48);
    static void pushBezier(tvg::Scene&, Vec2 a, Vec2 b, Color4 c, float sw);
};
