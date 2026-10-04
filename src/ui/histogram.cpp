#include "histogram.h"

void HistogramWidget::draw(tvg::Scene& scene) {
    Vec2 wp = worldPos();
    int  n  = (int)values.size();
    if (n == 0 || width <= 0.f) return;

    float bw = (width - kBarGap * (n - 1)) / n;

    // Background
    pushRect(scene, wp.x, wp.y, width, kH, 3.f, pal::SliderTrack);

    for (int i = 0; i < n; ++i) {
        float bh   = values[i] * (kH - 2.f);
        float bx   = wp.x + i * (bw + kBarGap);
        float by   = wp.y + (kH - bh);
        Color4 col = (i == activeBin) ? pal::HistBarHot : pal::HistBar;
        if (bh > 0.5f)
            pushRect(scene, bx, by, bw, bh, 1.f, col);
    }
}
