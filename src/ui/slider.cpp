#include "slider.h"
#include "node_canvas.h"
#include <cstdio>
#include <cmath>

void SliderWidget::draw(tvg::Scene& scene) {
    Vec2 wp = worldPos();
    float trackY = wp.y + kH - kTrackH - 3.f;
    float tx = wp.x + kPad;
    float tw = width - 2.f * kPad;

    // Label + value text
    if (NodeCanvas::fontLoaded()) {
        // Format value: use 0 decimals for large ints, 1 for medium, 2 for small
        char valBuf[32];
        float av = std::fabs(value);
        if (av >= 100.f)      std::snprintf(valBuf, sizeof(valBuf), "%.0f", value);
        else if (av >= 10.f)  std::snprintf(valBuf, sizeof(valBuf), "%.1f", value);
        else                  std::snprintf(valBuf, sizeof(valBuf), "%.2f", value);

        float textY = wp.y + 10.f;

        if (!label.empty()) {
            auto tl = tvg::Text::gen();
            if (tl->font(NodeCanvas::fontName().c_str(), 9.f) == tvg::Result::Success) {
                tl->text(label.c_str());
                tl->fill(180, 180, 200);
                tl->translate(tx, textY);
                scene.push(std::move(tl));
            }
        }

        auto tv = tvg::Text::gen();
        if (tv->font(NodeCanvas::fontName().c_str(), 9.f) == tvg::Result::Success) {
            tv->text(valBuf);
            tv->fill(220, 220, 240);
            float valW = std::strlen(valBuf) * 5.5f;
            tv->translate(wp.x + width - kPad - valW, textY);
            scene.push(std::move(tv));
        }
    }

    // Track background
    pushRect(scene, tx, trackY, tw, kTrackH, 2.f, pal::SliderTrack);

    // Filled portion
    float fill = norm() * tw;
    if (fill > 0.f)
        pushRect(scene, tx, trackY, fill, kTrackH, 2.f, pal::SliderFill);

    // Thumb
    float thumbX = tx + norm() * tw;
    float thumbY = trackY + kTrackH * 0.5f;
    pushCircle(scene, thumbX, thumbY, kThumbR, pal::SliderThumb, pal::NodeBorder, 1.f);
}
