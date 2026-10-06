#pragma once
#include "widget.h"
#include "node_canvas.h"
#include <functional>
#include <string>
#include <vector>

// A row of labelled toggle buttons — one active at a time.
class SelectorWidget final : public Widget {
public:
    std::vector<std::string>  options;
    int                       selected = 0;
    std::function<void(int)>  onChange;

    static constexpr float kBtnH = 22.f;
    static constexpr float kGap  =  3.f;

    SelectorWidget(std::vector<std::string> opts, int initial = 0)
        : options(std::move(opts)), selected(initial)
    {
        height = kBtnH;
        // width is set by the parent node (addWidget sets it to node inner width)
    }

    bool contains(Vec2 pt) const override {
        Vec2 wp = worldPos();
        return pt.x >= wp.x && pt.x <= wp.x + width
            && pt.y >= wp.y && pt.y <= wp.y + kBtnH;
    }

    void onDrag(Vec2) override {}
    void onMouseDown(Vec2 pt, int) override {
        if (options.empty()) return;
        Vec2  wp   = worldPos();
        float btnW = (width - kGap * (options.size() - 1)) / options.size();
        int   idx  = (int)((pt.x - wp.x) / (btnW + kGap));
        idx = std::max(0, std::min((int)options.size() - 1, idx));
        if (idx != selected) {
            selected = idx;
            if (onChange) onChange(selected);
        }
    }

    void draw(tvg::Scene& scene) override {
        if (options.empty()) return;
        Vec2  wp   = worldPos();
        int   n    = (int)options.size();
        float btnW = (width - kGap * (n - 1)) / n;

        for (int i = 0; i < n; ++i) {
            float bx = wp.x + i * (btnW + kGap);
            Color4 bg = (i == selected) ? pal::SliderFill
                                        : Color4{50, 50, 65, 255};
            pushRect(scene, bx, wp.y, btnW, kBtnH, 4.f, bg, pal::NodeBorder, 1.f);

            if (NodeCanvas::fontLoaded() && i < (int)options.size()) {
                auto t = tvg::Text::gen();
                if (t->font(NodeCanvas::fontName().c_str(), 9.f) == tvg::Result::Success) {
                    t->text(options[i].c_str());
                    Color4 tc = (i == selected) ? Color4{255,255,255,255}
                                                : Color4{160,160,180,255};
                    t->fill(tc.r, tc.g, tc.b);
                    t->translate(bx + 4.f, wp.y + kBtnH * 0.5f - 5.f);
                    scene.push(std::move(t));
                }
            }
        }
    }

    nlohmann::json toJson()  const override { return selected; }
    void fromJson(const nlohmann::json& j) override {
        if (j.is_number_integer()) {
            selected = std::max(0, std::min((int)options.size()-1, j.get<int>()));
        }
    }
};
