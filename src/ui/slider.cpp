#include "slider.h"
#include "node_canvas.h"
#include <SDL3/SDL.h>
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

        float textY = wp.y + 8.f;

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

    // Thumb — brighter when shift-snap is active
    bool shifting = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
    Color4 thumbCol = shifting ? Color4{220, 240, 255, 255} : pal::SliderThumb;
    float thumbX = tx + norm() * tw;
    float thumbY = trackY + kTrackH * 0.5f;
    pushCircle(scene, thumbX, thumbY, kThumbR, thumbCol, pal::NodeBorder, 1.f);
}

float SliderWidget::stepSize() const {
    float range = std::fabs(max - min);
    if (range < 1e-6f) return 1.f;
    float raw = range / 100.f;
    float mag = std::pow(10.f, std::floor(std::log10(raw)));
    float ratio = raw / mag;
    if (ratio >= 5.f) return mag * 5.f;
    if (ratio >= 2.f) return mag * 2.f;
    return mag;
}

void SliderWidget::setFromX(float worldX) {
    Vec2  wp = worldPos();
    float t  = (worldX - (wp.x + kPad)) / (width - 2.f * kPad);
    float v  = min + clamp01(t) * (max - min);

    if (SDL_GetModState() & SDL_KMOD_SHIFT) {
        float step = stepSize();
        v = std::round(v / step) * step;
    }

    v = clampf(v, min, max);
    if (v != value) { value = v; if (onChange) onChange(v); }
}
