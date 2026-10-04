#include "slider.h"

void SliderWidget::draw(tvg::Scene& scene) {
    Vec2 wp = worldPos();
    float ty = wp.y + (kH - kTrackH) * 0.5f;
    float tx = wp.x + kPad;
    float tw = width - 2.f * kPad;

    // Track background
    pushRect(scene, tx, ty, tw, kTrackH, 2.f, pal::SliderTrack);

    // Filled portion
    float fill = norm() * tw;
    if (fill > 0.f)
        pushRect(scene, tx, ty, fill, kTrackH, 2.f, pal::SliderFill);

    // Thumb
    float tx2 = tx + norm() * tw;
    float ty2 = wp.y + kH * 0.5f;
    pushCircle(scene, tx2, ty2, kThumbR, pal::SliderThumb, pal::NodeBorder, 1.f);
}
